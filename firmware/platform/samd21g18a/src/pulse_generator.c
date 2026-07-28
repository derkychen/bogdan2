#include "platform/samd21g18a/pulse_generator.h"
#include "platform/samd21g18a/assert.h"
#include "platform/samd21g18a/pin.h"
#include "platform/samd21g18a/utils.h"
#include "sam.h" // IWYU pragma: keep
#include <stdbool.h>
#include <stdint.h>

#define TCC0_PULSE_WIDTH_TICKS_MAX     (0x00FFFFFEu)
#define TCC0_PULSE_WIDTH_INITIAL_TICKS (1u)
#define TCC0_EVSYS_CHANNEL             (0U)

static platform_samd21g18a_pin_t const pa04 = {
    .port_group = PLATFORM_SAMD21G18A_PIN_PORT_GROUP_A,
    .number     = 4u,
};

static void tcc0_poll_sync(uint32_t mask);

static void tcc0_width_set(uint32_t width_ticks);

static void tcc0_evsys_route_disable(void);

static void tcc0_evsys_route_enable(platform_samd21g18a_eic_extint_line_t line);

void
platform_samd21g18a_pulse_generator_tcc0_init (void)
{
    platform_samdreg18a_pin_output_hold_low(&pa04);

    // NOTE: TCC0 and TCC1 use the same generic clock channel.
    PM->APBCMASK.reg |= PM_APBCMASK_TCC0;
    PM->APBCMASK.reg |= PM_APBCMASK_EVSYS;

    platform_samd21g18a_utils_gclk0_enable(GCLK_CLKCTRL_ID_TCC0_TCC1);
    platform_samd21g18a_utils_gclk_poll_sync();

    TCC0->CTRLA.reg = TCC_CTRLA_SWRST;

    while (((TCC0->SYNCBUSY.reg & TCC_SYNCBUSY_SWRST) != 0U)
           || ((TCC0->CTRLA.reg & TCC_CTRLA_SWRST) != 0U))
    {
    }

    TCC0->CTRLA.reg = TCC_CTRLA_PRESCALER_DIV1 | TCC_CTRLA_PRESCSYNC_GCLK;

    TCC0->WAVE.reg = TCC_WAVE_WAVEGEN_NPWM;
    tcc0_poll_sync(TCC_SYNCBUSY_WAVE);

    // This keeps PA04 LOW between pulses.
    TCC0->DRVCTRL.reg = TCC_DRVCTRL_NRE0;

    // NOTE: Configure RETRIGGER before enabling TCC0 ensures ENABLE does not
    //       start the counter so that what starts it is the first event or
    //       software re-trigger.
    TCC0->EVCTRL.reg = TCC_EVCTRL_TCEI0 | TCC_EVCTRL_EVACT0_RETRIGGER;

    TCC0->COUNT.reg = 0U;
    tcc0_poll_sync(TCC_SYNCBUSY_COUNT);

    tcc0_width_set(TCC0_PULSE_WIDTH_INITIAL_TICKS);

    TCC0->CTRLBSET.reg = TCC_CTRLBSET_ONESHOT;
    tcc0_poll_sync(TCC_SYNCBUSY_CTRLB);

    // Clear flags.
    TCC0->INTFLAG.reg = TCC_INTFLAG_OVF | TCC_INTFLAG_TRG | TCC_INTFLAG_MC0;

    TCC0->CTRLA.reg |= TCC_CTRLA_ENABLE;
    tcc0_poll_sync(TCC_SYNCBUSY_ENABLE);

    while ((TCC0->STATUS.reg & TCC_STATUS_STOP) == 0U)
    {
    }

    // Connect PA04 to the TCC0 peripheral function (E).
    platform_samd21g18a_pin_set_peripheral_function(
        &pa04, PLATFORM_SAMD21G18A_PIN_PERIPHERAL_FUNCTION_E);

    return;
}

void
platform_samd21g18a_pulse_generator_tcc0_event_disable (
    platform_samd21g18a_eic_extint_line_t line)
{
    PLATFORM_SAMD21G18A_ASSERT(line
                               < PLATFORM_SAMD21G18A_EIC_EXTINT_LINE_COUNT);

    platform_samd21g18a_eic_event_output_disable(line);

    tcc0_evsys_route_disable();

    return;
}

void
platform_samd21g18a_pulse_generator_tcc0_event_enable (
    platform_samd21g18a_eic_extint_line_t line)
{
    PLATFORM_SAMD21G18A_ASSERT(line
                               < PLATFORM_SAMD21G18A_EIC_EXTINT_LINE_COUNT);

    platform_samd21g18a_eic_event_output_enable(line);

    tcc0_evsys_route_enable(line);

    return;
}

void
platform_samd21g18a_pulse_generator_tcc0_width_set (uint32_t width_ticks)
{
    PLATFORM_SAMD21G18A_ASSERT(width_ticks > 0U);
    PLATFORM_SAMD21G18A_ASSERT(width_ticks <= TCC0_PULSE_WIDTH_TICKS_MAX);

    // Ensure that width setting does not occur when pulses are firing.
    PLATFORM_SAMD21G18A_ASSERT((TCC0->STATUS.reg & TCC_STATUS_STOP) != 0U);

    TCC0->COUNT.reg = 0U;
    tcc0_poll_sync(TCC_SYNCBUSY_COUNT);

    tcc0_width_set(width_ticks);

    return;
}

void
platform_samd21g18a_pulse_generator_tcc0_retrigger (void)
{
    tcc0_poll_sync(TCC_SYNCBUSY_CTRLB);

    TCC0->CTRLBSET.reg = TCC_CTRLBSET_CMD_RETRIGGER;

    tcc0_poll_sync(TCC_SYNCBUSY_CTRLB);

    return;
}

/** @brief Poll until TCC0 is synchronized. */
static void
tcc0_poll_sync (uint32_t mask)
{
    while ((TCC0->SYNCBUSY.reg & mask) != 0U)
    {
    }

    return;
}

/** @brief Set the width of the TCC0 one-shot pulse. */
static void
tcc0_width_set (uint32_t width_ticks)
{
    // NOTE: CCO is how many cycles the pin is HIGH for. PER is one tick longer
    //       so that the falling edge and the overflow indicating the pulse is
    //       over do not coincide.
    TCC0->PER.reg = width_ticks + 1u;
    tcc0_poll_sync(TCC_SYNCBUSY_PER);

    TCC0->CC[0U].reg = width_ticks;
    tcc0_poll_sync(TCC_SYNCBUSY_CC0);

    return;
}

/** @brief Disable an EVSYS route. */
static void
tcc0_evsys_route_disable (void)
{
    EVSYS->USER.reg = EVSYS_USER_USER(EVSYS_ID_USER_TCC0_EV_0)
                      | EVSYS_USER_CHANNEL(TCC0_EVSYS_CHANNEL);

    EVSYS->CHANNEL.reg
        = EVSYS_CHANNEL_CHANNEL(TCC0_EVSYS_CHANNEL) | EVSYS_CHANNEL_EVGEN(0u);

    return;
}

/** @brief Enable an EVSYS route. */
static void
tcc0_evsys_route_enable (platform_samd21g18a_eic_extint_line_t line)
{
    EVSYS->USER.reg = EVSYS_USER_USER(EVSYS_ID_USER_TCC0_EV_0)
                      | EVSYS_USER_CHANNEL(TCC0_EVSYS_CHANNEL + 1u);

    EVSYS->CHANNEL.reg
        = EVSYS_CHANNEL_CHANNEL(TCC0_EVSYS_CHANNEL)
          | EVSYS_CHANNEL_EVGEN(
              (uint16_t)(EVSYS_ID_GEN_EIC_EXTINT_0 + (uint32_t)line))
          | EVSYS_CHANNEL_PATH_ASYNCHRONOUS;

    return;
}
