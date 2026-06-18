/** @brief System startup functionality.
 *
 * This module just configures the system clock to 48 MHz. As such,
 * `SystemCoreClockUpdate` only sets SystemCoreClock to 48 000 000 after the
 * system is initialized, and does not calculate the clock frequency. Although
 * there is no use case for this, if clock frequency is changed elsewhere,
 * `SystemCoreClockUpdate` will have to be rewritten.
 */
#include "samd21g18a.h"
#include "system_samd21g18a.h"
#include <stdbool.h>

#define SYSTEM_CORE_CLOCK_FREQUENCY_RESET_HZ 1000000u
#define SYSTEM_CORE_CLOCK_FREQUENCY_INIT_HZ  48000000u

#define SAMD21G18A_XOSC32K_FREQUENCY_HZ           32768u
#define SAMD21G18A_DFLL48M_MULTIPLIER             1465u
#define SAMD21G18A_DFLL48M_FINE_STEP              511u
#define SAMD21G18A_DFLL48M_COARSE_STEP            31u
#define SAMD21G18A_DFLL48M_COARSE_CAL_INVALID     0x3fu
#define SAMD21G18A_DFLL48M_COARSE_CAL_FALLBACK    0x1fu
#define SAMD21G18A_NVM_SW_CALIBRATION_WORD_1_ADDR 0x00806024u

uint32_t    SystemCoreClock          = SYSTEM_CORE_CLOCK_FREQUENCY_RESET_HZ;
static bool system_clock_initialized = false;

/** @brief Poll the XOSC32K ready status bit until it is ready. */
static inline void
poll_xosc32k_until_ready (void)
{
    while ((SYSCTRL_REGS->SYSCTRL_PCLKSR & SYSCTRL_PCLKSR_XOSC32KRDY_Msk) == 0u)
    {
    }

    return;
}

/** @brief Poll the GCLK register until it is synchronized. */
static inline void
poll_gclk_until_synchronized (void)
{
    while ((GCLK_REGS->GCLK_STATUS & GCLK_STATUS_SYNCBUSY_Msk) != 0u)
    {
    }

    return;
}

/** @brief Poll the DFLL register until it is ready. */
static inline void
poll_dfll_until_ready (void)
{
    while ((SYSCTRL_REGS->SYSCTRL_PCLKSR & SYSCTRL_PCLKSR_DFLLRDY_Msk) == 0u)
    {
    }

    return;
}

/** @brief Poll the DFLL register until clock frequency has been locked. */
static inline void
poll_dfll_until_locked (void)
{
    while (
        ((SYSCTRL_REGS->SYSCTRL_PCLKSR & SYSCTRL_PCLKSR_DFLLLCKC_Msk) == 0u)
        || ((SYSCTRL_REGS->SYSCTRL_PCLKSR & SYSCTRL_PCLKSR_DFLLLCKF_Msk) == 0u))
    {
    }

    return;
}

/** @brief Set number of wait states for 48 MHz at 3.3V logic. */
static void
set_number_of_wait_states_48_mhz (void)
{
    NVMCTRL_REGS->NVMCTRL_CTRLB
        = (NVMCTRL_REGS->NVMCTRL_CTRLB & ~NVMCTRL_CTRLB_RWS_Msk)
          | NVMCTRL_CTRLB_RWS(1u);

    return;
}

/** @brief Start up and wait for oscillator. */
static void
xosc32k_start_and_enable (void)
{
    // TODO: Currently using the longest start-up time option (~4 s). Switch to
    // a shorter one once tested.
    SYSCTRL_REGS->SYSCTRL_XOSC32K
        = SYSCTRL_XOSC32K_STARTUP(SYSCTRL_XOSC32K_STARTUP_CYCLE131072_Val)
          | SYSCTRL_XOSC32K_EN32K_Msk | SYSCTRL_XOSC32K_XTALEN_Msk;

    // Enable the oscillator in a separate write as per the data sheet.
    SYSCTRL_REGS->SYSCTRL_XOSC32K |= SYSCTRL_XOSC32K_ENABLE_Msk;
    poll_xosc32k_until_ready();

    return;
}

/**
 * @brief Route the oscillator through GCLK1 undivided and improve duty cycle.
 */
static void
gclk1_set_source_to_xosc32k (void)
{
    GCLK_REGS->GCLK_GENDIV  = GCLK_GENDIV_ID(1u) | GCLK_GENDIV_DIV(1u);
    GCLK_REGS->GCLK_GENCTRL = GCLK_GENCTRL_ID(1u) | GCLK_GENCTRL_SRC_XOSC32K
                              | GCLK_GENCTRL_IDC_Msk | GCLK_GENCTRL_GENEN_Msk;
    poll_gclk_until_synchronized();

    return;
}

/** @brief Configure the DFLL reference clock to be the output of GCLK1. */
static void
dfll_set_reference_to_gclk1 (void)
{
    GCLK_REGS->GCLK_CLKCTRL = GCLK_CLKCTRL_ID_DFLL48 | GCLK_CLKCTRL_GEN_GCLK1
                              | GCLK_CLKCTRL_CLKEN_Msk;
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
    SYSCTRL_REGS->SYSCTRL_DFLLCTRL = SYSCTRL_DFLLCTRL_ENABLE_Msk;
    poll_dfll_until_ready();

    // DFLL multiplies the oscillator frequency to the desired 48 MHz clock
    // frequency. The coarse and fine step values specified here are half
    // their maximum value. These values are used by the DFLL to lock onto
    // the desired frequency. Higher values mean less overshoot but longer
    // time and vice versa.
    SYSCTRL_REGS->SYSCTRL_DFLLMUL
        = SYSCTRL_DFLLMUL_MUL(SAMD21G18A_DFLL48M_MULTIPLIER)
          | SYSCTRL_DFLLMUL_FSTEP(SAMD21G18A_DFLL48M_FINE_STEP)
          | SYSCTRL_DFLLMUL_CSTEP(SAMD21G18A_DFLL48M_COARSE_STEP);

    // Load factory-programmed coarse calibration value or fallback into
    // `DFLLVAL.COARSE` for faster locking.
    coarse = ((*((volatile const uint32_t *)
                     SAMD21G18A_NVM_SW_CALIBRATION_WORD_1_ADDR)
               & FUSES_OTP4_WORD_1_DFLL48M_COARSE_CAL_Msk)
              >> FUSES_OTP4_WORD_1_DFLL48M_COARSE_CAL_Pos);

    if (coarse == SAMD21G18A_DFLL48M_COARSE_CAL_INVALID)
    {
        coarse = SAMD21G18A_DFLL48M_COARSE_CAL_FALLBACK;
    }

    SYSCTRL_REGS->SYSCTRL_DFLLVAL
        = (SYSCTRL_REGS->SYSCTRL_DFLLVAL & ~SYSCTRL_DFLLVAL_COARSE_Msk)
          | SYSCTRL_DFLLVAL_COARSE(coarse);

    poll_dfll_until_ready();

    // Set the DFLL to closed-loop mode. Configure the DFLL to only output
    // when the frequency is locked and enable.
    SYSCTRL_REGS->SYSCTRL_DFLLCTRL = SYSCTRL_DFLLCTRL_MODE_Msk
                                     | SYSCTRL_DFLLCTRL_WAITLOCK_Msk
                                     | SYSCTRL_DFLLCTRL_ENABLE_Msk;

    // Wait for the clock frequency to lock.
    poll_dfll_until_locked();

    return;
}

/** @brief Route DFLL output through GCLK0 and improve duty cycle. */
static void
gclk0_set_source_to_dfll (void)
{
    GCLK_REGS->GCLK_GENCTRL = GCLK_GENCTRL_ID(0u) | GCLK_GENCTRL_SRC_DFLL48M
                              | GCLK_GENCTRL_IDC_Msk | GCLK_GENCTRL_GENEN_Msk;
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
