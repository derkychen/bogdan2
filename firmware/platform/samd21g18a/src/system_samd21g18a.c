/** @brief System startup functionality.
 *
 * This module just configures the system clock to 48 MHz. As such,
 * `SystemCoreClockUpdate` only sets SystemCoreClock to 48 000 000 after the
 * system is initialized, and does not calculate the clock frequency. Although
 * there is no use case for this, if clock frequency is changed elsewhere,
 * `SystemCoreClockUpdate` will have to be rewritten.
 */
#include "samd21/include/samd21g18a.h"
#include "samd21/include/system_samd21.h"
#include <stdbool.h>
#include <stdint.h>

#define SYSTEM_CORE_CLOCK_FREQUENCY_RESET_HZ (1000000u)
#define SYSTEM_CORE_CLOCK_FREQUENCY_INIT_HZ  (48000000u)

#define SAMD21G18A_XOSC32K_FREQUENCY_HZ        (32768u)
#define SYSCTRL_XOSC32K_STARTUP_CYCLE131072    (0x7u)
#define SAMD21G18A_DFLL48M_MULTIPLIER          (1465u)
#define SAMD21G18A_DFLL48M_FINE_STEP           (511u)
#define SAMD21G18A_DFLL48M_COARSE_STEP         (31u)
#define SAMD21G18A_DFLL48M_COARSE_CAL_INVALID  (0x3fu)
#define SAMD21G18A_DFLL48M_COARSE_CAL_FALLBACK (0x1fu)

uint32_t    SystemCoreClock          = SYSTEM_CORE_CLOCK_FREQUENCY_RESET_HZ;
static bool system_clock_initialized = false;

/** @brief Poll the XOSC32K ready status bit until it is ready. */
static inline void
poll_xosc32k_until_ready (void)
{
    while (!SYSCTRL->PCLKSR.bit.XOSC32KRDY)
    {
    }

    return;
}

/** @brief Poll the GCLK register until it is synchronized. */
static inline void
poll_gclk_until_synchronized (void)
{
    while (GCLK->STATUS.bit.SYNCBUSY)
    {
    }

    return;
}

/** @brief Poll the DFLL register until it is ready. */
static inline void
poll_dfll_until_ready (void)
{
    while (!SYSCTRL->PCLKSR.bit.DFLLRDY)
    {
    }

    return;
}

/** @brief Poll the DFLL register until clock frequency has been locked. */
static inline void
poll_dfll_until_locked (void)
{
    while (!SYSCTRL->PCLKSR.bit.DFLLLCKC || !SYSCTRL->PCLKSR.bit.DFLLLCKF)
    {
    }

    return;
}

/** @brief Set number of wait states for 48 MHz at 3.3V logic. */
static void
set_number_of_wait_states_48_mhz (void)
{
    NVMCTRL->CTRLB.bit.RWS = 1u;

    return;
}

/** @brief Start up and wait for oscillator. */
static void
xosc32k_start_and_enable (void)
{
    // TODO: Currently using the longest start-up time option (~4 s). Switch to
    // a shorter one once tested.
    SYSCTRL->XOSC32K.reg
        = SYSCTRL_XOSC32K_STARTUP(SYSCTRL_XOSC32K_STARTUP_CYCLE131072)
          | SYSCTRL_XOSC32K_EN32K | SYSCTRL_XOSC32K_XTALEN;

    // Enable the oscillator in a separate write as per the data sheet.
    SYSCTRL->XOSC32K.bit.ENABLE = 1u;

    poll_xosc32k_until_ready();

    return;
}

/** @brief Set GCLK1's source to the oscillator and improve its duty cycle. */
static void
gclk1_set_source_to_xosc32k (void)
{
    GCLK->GENDIV.reg = GCLK_GENDIV_ID(1u) | GCLK_GENDIV_DIV(1u);

    GCLK->GENCTRL.reg = GCLK_GENCTRL_ID(1u) | GCLK_GENCTRL_SRC_XOSC32K
                        | GCLK_GENCTRL_IDC | GCLK_GENCTRL_GENEN;

    poll_gclk_until_synchronized();

    return;
}

/** @brief Set the DFLL reference clock to be the output of GCLK1. */
static void
dfll_set_reference_to_gclk1 (void)
{
    GCLK->CLKCTRL.reg
        = GCLK_CLKCTRL_ID_DFLL48 | GCLK_CLKCTRL_GEN_GCLK1 | GCLK_CLKCTRL_CLKEN;

    poll_gclk_until_synchronized();

    return;
}

/** @brief Lock the DFLL onto the desired 48 MHz clock frequency. */
static void
dfll_lock_48_mhz (void)
{
    uint32_t coarse;

    // This is a workaround for a hardware quirk in which the `DFLLCTRL`
    // register must be reset to this value before configuration.
    poll_dfll_until_ready();
    SYSCTRL->DFLLCTRL.reg = SYSCTRL_DFLLCTRL_ENABLE;
    poll_dfll_until_ready();

    // DFLL multiplies the oscillator frequency to the desired 48 MHz clock
    // frequency. The coarse and fine step values specified here are half
    // their maximum value. These values are used by the DFLL to lock onto
    // the desired frequency. Higher values mean less overshoot but longer
    // time and vice versa.
    SYSCTRL->DFLLMUL.reg
        = SYSCTRL_DFLLMUL_MUL(SAMD21G18A_DFLL48M_MULTIPLIER)
          | SYSCTRL_DFLLMUL_FSTEP(SAMD21G18A_DFLL48M_FINE_STEP)
          | SYSCTRL_DFLLMUL_CSTEP(SAMD21G18A_DFLL48M_COARSE_STEP);

    // Load factory-programmed coarse calibration value or fallback into
    // `DFLLVAL.COARSE` for faster locking.
    coarse = (*((volatile const uint32_t *)FUSES_DFLL48M_COARSE_CAL_ADDR)
              & FUSES_DFLL48M_COARSE_CAL_Msk)
             >> FUSES_DFLL48M_COARSE_CAL_Pos;

    if (coarse == SAMD21G18A_DFLL48M_COARSE_CAL_INVALID)
    {
        coarse = SAMD21G18A_DFLL48M_COARSE_CAL_FALLBACK;
    }

    SYSCTRL->DFLLVAL.bit.COARSE = coarse;

    poll_dfll_until_ready();

    // Set the DFLL to closed-loop mode. Configure the DFLL to only output
    // when the frequency is locked and enable.
    SYSCTRL->DFLLCTRL.reg |= SYSCTRL_DFLLCTRL_MODE | SYSCTRL_DFLLCTRL_WAITLOCK
                             | SYSCTRL_DFLLCTRL_ENABLE;

    poll_dfll_until_locked();

    return;
}

/** @brief Set GCLK0's source to the output of DFLL and improve duty cycle. */
static void
gclk0_set_source_to_dfll (void)
{
    GCLK->GENCTRL.reg = GCLK_GENCTRL_ID(0) | GCLK_GENCTRL_SRC_DFLL48M
                        | GCLK_GENCTRL_IDC | GCLK_GENCTRL_GENEN;

    poll_gclk_until_synchronized();

    return;
}

void
SystemInit (void)
{
    set_number_of_wait_states_48_mhz();
    xosc32k_start_and_enable();
    gclk1_set_source_to_xosc32k();
    dfll_set_reference_to_gclk1();
    dfll_lock_48_mhz();
    gclk0_set_source_to_dfll();

    SystemCoreClock          = SYSTEM_CORE_CLOCK_FREQUENCY_INIT_HZ;
    system_clock_initialized = true;

    return;
}

void
SystemCoreClockUpdate (void)
{
    if (system_clock_initialized)
    {
        SystemCoreClock = SYSTEM_CORE_CLOCK_FREQUENCY_INIT_HZ;
    }
    else
    {
        SystemCoreClock = SYSTEM_CORE_CLOCK_FREQUENCY_RESET_HZ;
    }

    return;
}
