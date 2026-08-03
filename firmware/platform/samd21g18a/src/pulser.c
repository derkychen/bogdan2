/**
 * @file pulser.c
 * @brief Implementation of the one-shot pulse generating module.
 *
 * NOTE: This implementation only supports TCC timers.
 */
#include "platform/samd21g18a/pulser.h"
#include "platform/samd21g18a/assert.h"
#include "platform/samd21g18a/pin.h"
#include "platform/samd21g18a/utils.h"
#include "sam.h" // IWYU pragma: keep
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PULSER_WIDTH_TICKS_MAX (0x00FFFFFEu)

/** @brief Supported pulser route. */
typedef struct
{
    /** Output pin port group. */
    pin_port_group_t pin_port_group;

    /** Output pin number. */
    pin_number_t pin_number;

    /** Peripheral function for the output. */
    pin_peripheral_function_t pin_peripheral_function;

    /** Pulser hardware timer instance. */
    pulser_timer_t timer;
} route_t;

/** @brief Hardware data associated with a pulser. */
typedef struct
{
    /** TCC registers. */
    Tcc *tcc;

    /** APBC mask. */
    uint32_t apbc_mask;

    /** GCLK identifier for the TCC instance. */
    uint16_t gclk_id;

    /** Compare channel for output pulse width. */
    uint8_t compare_channel;

    /** Synchronization mask for the compare channel. */
    uint32_t compare_sync_mask;

    /** Match interrupt flag for the compare channel. */
    uint32_t match_flag;

    /** Mask enabling the non-recoverable fault output value. */
    uint32_t non_recoverable_enable_mask;

    /** Event user corresponding to the timer event input. */
    evsys_user_t event_user;
} timer_data_t;

// NOTE: Only TCC0 is supported.
static route_t const routes[] = {
    {
        .pin_port_group          = PIN_PORT_GROUP_A,
        .pin_number              = 4u,
        .pin_peripheral_function = PIN_PERIPHERAL_FUNCTION_E,
        .timer                   = PULSER_TIMER_TCC0,
    },
};

static timer_data_t const timer_data[PULSER_TIMER_COUNT] = {
    [PULSER_TIMER_TCC0] = {
        .tcc                         = TCC0,
        .apbc_mask                   = PM_APBCMASK_TCC0,
        .gclk_id                     = GCLK_CLKCTRL_ID_TCC0_TCC1,
        .compare_channel             = 0u,
        .compare_sync_mask           = TCC_SYNCBUSY_CC0,
        .match_flag                  = TCC_INTFLAG_MC0,
        .non_recoverable_enable_mask = TCC_DRVCTRL_NRE0,
        .event_user                  =
            (evsys_user_t)EVSYS_ID_USER_TCC0_EV_0,
    },
};

static inline bool               timer_valid(pulser_timer_t timer);
static inline Tcc               *timer_get_tcc(pulser_timer_t timer);
static void                      tcc_poll_sync(Tcc *tcc, uint32_t mask);
static bool                      tcc_stopped(Tcc *tcc);
static pin_peripheral_function_t output_get_timer_peripheral_function(
    pulser_output_t const *output, pulser_timer_t timer);

void
pulser_configure (pulser_t const *pulser)
{
    ASSERT(pulser != NULL);
    ASSERT(pulser->output != NULL);
    ASSERT(pin_port_group_valid(pulser->output->port_group));
    ASSERT(pin_number_valid(pulser->output->number));
    ASSERT(timer_valid(pulser->timer));

    // Hold the pin LOW to safely configure it.
    pin_output_hold_low(pulser->output);

    timer_data_t data = timer_data[pulser->timer];

    utils_apbc_enable(data.apbc_mask);

    // TCC0 and TCC1 share the same GCLK ID.
    utils_gclk0_enable(data.gclk_id);

    Tcc *tcc = data.tcc;

    // Reset the count.
    tcc->CTRLA.reg = TCC_CTRLA_SWRST;

    while (((tcc->SYNCBUSY.reg & TCC_SYNCBUSY_SWRST) != 0u)
           || ((tcc->CTRLA.reg & TCC_CTRLA_SWRST) != 0u))
    {
    }

    // Use GCLK directly without any prescaler.
    tcc->CTRLA.reg = TCC_CTRLA_PRESCALER_DIV1 | TCC_CTRLA_PRESCSYNC_GCLK;

    // Use PWM waveform generation.
    tcc->WAVE.reg = TCC_WAVE_WAVEGEN_NPWM;
    tcc_poll_sync(tcc, TCC_SYNCBUSY_WAVE);

    // This keeps the pulser LOW between pulses.
    tcc->DRVCTRL.reg = data.non_recoverable_enable_mask;

    // Configure TCC event input zero to re-trigger the one-shot pulse.
    tcc->EVCTRL.reg = TCC_EVCTRL_TCEI0 | TCC_EVCTRL_EVACT0_RETRIGGER;

    tcc->COUNT.reg = 0u;
    tcc_poll_sync(tcc, TCC_SYNCBUSY_COUNT);

    pulser_width_set(pulser, 1u);

    tcc->CTRLBSET.reg = TCC_CTRLBSET_ONESHOT;
    tcc_poll_sync(tcc, TCC_SYNCBUSY_CTRLB);
    tcc->INTFLAG.reg = TCC_INTFLAG_OVF | TCC_INTFLAG_TRG | data.match_flag;

    // NOTE: Configuring RETRIGGER before enabling TCC0 ensures ENABLE does not
    //       start the counter so that what starts it is the first event or
    //       software re-trigger.
    tcc->CTRLA.reg |= TCC_CTRLA_ENABLE;
    tcc_poll_sync(tcc, TCC_SYNCBUSY_ENABLE);

    while (!tcc_stopped(tcc))
    {
    }

    pin_set_peripheral_function(
        pulser->output,
        output_get_timer_peripheral_function(pulser->output, pulser->timer));

    return;
}

void
pulser_width_set (pulser_t const *pulser, uint32_t width_ticks)
{
    ASSERT(pulser != NULL);
    ASSERT(timer_valid(pulser->timer));
    ASSERT(width_ticks > 0u);
    ASSERT(width_ticks <= PULSER_WIDTH_TICKS_MAX);

    Tcc *tcc = timer_get_tcc(pulser->timer);

    ASSERT(tcc_stopped(tcc));

    tcc->COUNT.reg = 0u;
    tcc_poll_sync(tcc, TCC_SYNCBUSY_COUNT);

    // NOTE: CC is how many cycles the pin is HIGH for. PER is one tick longer
    //       so that the falling edge and the overflow indicating the pulse is
    //       over do not coincide.
    tcc->PER.reg = width_ticks + 1u;
    tcc_poll_sync(tcc, TCC_SYNCBUSY_PER);

    tcc->CC[timer_data[pulser->timer].compare_channel].reg = width_ticks;
    tcc_poll_sync(tcc, timer_data[pulser->timer].compare_sync_mask);

    return;
}

void
pulser_retrigger (pulser_t const *pulser)
{
    ASSERT(pulser != NULL);
    ASSERT(timer_valid(pulser->timer));

    Tcc *tcc = timer_get_tcc(pulser->timer);

    tcc_poll_sync(tcc, TCC_SYNCBUSY_CTRLB);

    tcc->CTRLBSET.reg = TCC_CTRLBSET_CMD_RETRIGGER;

    tcc_poll_sync(tcc, TCC_SYNCBUSY_CTRLB);

    return;
}

evsys_user_t
pulser_event_user (pulser_t const *pulser)
{
    ASSERT(pulser != NULL);
    ASSERT(timer_valid(pulser->timer));

    return timer_data[pulser->timer].event_user;
}

/** @brief Check whether a pulser instance is valid. */
static inline bool
timer_valid (pulser_timer_t timer)
{
    return timer < PULSER_TIMER_COUNT;
}

/** @brief Get the TCC registers corresponding to a pulser. */
static inline Tcc *
timer_get_tcc (pulser_timer_t timer)
{
    ASSERT(timer_valid(timer));

    return timer_data[timer].tcc;
}

/** @brief Poll until a TCC until it is synchronized. */
static void
tcc_poll_sync (Tcc *tcc, uint32_t mask)
{
    ASSERT(tcc != NULL);

    while ((tcc->SYNCBUSY.reg & mask) != 0u)
    {
    }

    return;
}

/** @brief Check whether a TCC counter is stopped. */
static bool
tcc_stopped (Tcc *tcc)
{
    ASSERT(tcc != NULL);

    return (tcc->STATUS.reg & TCC_STATUS_STOP) != 0u;
}

/** @brief Get the peripheral function for an output for a supported route. */
static pin_peripheral_function_t
output_get_timer_peripheral_function (pulser_output_t const *output,
                                      pulser_timer_t         timer)
{
    ASSERT(output != NULL);
    ASSERT(timer_valid(timer));

    for (size_t index = 0u; index < sizeof(routes) / sizeof(routes[0]); index++)
    {
        if ((routes[index].timer == timer)
            && (routes[index].pin_port_group == output->port_group)
            && (routes[index].pin_number == output->number))
        {
            return routes[index].pin_peripheral_function;
        }
    }

    ASSERT(false);

    return PIN_PERIPHERAL_FUNCTION_A;
}
