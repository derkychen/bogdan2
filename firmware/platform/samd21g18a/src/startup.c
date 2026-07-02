#include "sam.h" // IWYU pragma: keep
#include <stdint.h>

// Bounds of segments in memory as initialized by linker script.
extern uint32_t _sfixed;
extern uint32_t _efixed;
extern uint32_t _etext;
extern uint32_t _srelocate;
extern uint32_t _erelocate;
extern uint32_t _szero;
extern uint32_t _ezero;
extern uint32_t _sstack;
extern uint32_t _estack;

/** @brief Main function defined elsewhere. */
extern int main(void);

/** @brief Processor reset handler. */
void Reset_Handler(void);

/**
 * @brief Dummy processor exception handler that does nothing.
 *
 * All exceptions are handled by it unless otherwise specified.
 */
void Dummy_Handler(void);

// Cortex-M0+ core and peripherals handlers.
//
// NOTE: Weak wrappers were used instead of aliases due aliases not being
//       supported on Darwin (macOS).
//
// TODO: Switch to aliases if Darwin support is deemed not needed to optimize
//       binary size. This will be likely when rapid development has ended.
#if 0
// CORTEX-M0+ handlers.
void NMI_Handler(void) __attribute__((weak, alias("Dummy_Handler")));
void *ardFault_Handler(void) __attribute__((weak, alias("Dummy_Handler")));
void SVC_Handler(void) __attribute__((weak, alias("Dummy_Handler")));
void PendSV_Handler(void) __attribute__((weak, alias("Dummy_Handler")));
void SysTick_Handler(void) __attribute__((weak, alias("Dummy_Handler")));

// Peripheral handlers.
void PM_Handler(void) __attribute__((weak, alias("Dummy_Handler")));
void SYSCTRL_Handler(void) __attribute__((weak, alias("Dummy_Handler")));
void WDT_Handler(void) __attribute__((weak, alias("Dummy_Handler")));
void RTC_Handler(void) __attribute__((weak, alias("Dummy_Handler")));
void EIC_Handler(void) __attribute__((weak, alias("Dummy_Handler")));
void NVMCTRL_Handler(void) __attribute__((weak, alias("Dummy_Handler")));
void DMAC_Handler(void) __attribute__((weak, alias("Dummy_Handler")));
void USB_Handler(void) __attribute__((weak, alias("Dummy_Handler")));
void EVSYS_Handler(void) __attribute__((weak, alias("Dummy_Handler")));
void SERCOM0_Handler(void) __attribute__((weak, alias("Dummy_Handler")));
void SERCOM1_Handler(void) __attribute__((weak, alias("Dummy_Handler")));
void SERCOM2_Handler(void) __attribute__((weak, alias("Dummy_Handler")));
void SERCOM3_Handler(void) __attribute__((weak, alias("Dummy_Handler")));
void SERCOM4_Handler(void) __attribute__((weak, alias("Dummy_Handler")));
void SERCOM5_Handler(void) __attribute__((weak, alias("Dummy_Handler")));
void TCC0_Handler(void) __attribute__((weak, alias("Dummy_Handler")));
void TCC1_Handler(void) __attribute__((weak, alias("Dummy_Handler")));
void TCC2_Handler(void) __attribute__((weak, alias("Dummy_Handler")));
void TC3_Handler(void) __attribute__((weak, alias("Dummy_Handler")));
void TC4_Handler(void) __attribute__((weak, alias("Dummy_Handler")));
void TC5_Handler(void) __attribute__((weak, alias("Dummy_Handler")));
void ADC_Handler(void) __attribute__((weak, alias("Dummy_Handler")));
void AC_Handler(void) __attribute__((weak, alias("Dummy_Handler")));
void DAC_Handler(void) __attribute__((weak, alias("Dummy_Handler")));
void PTC_Handler(void) __attribute__((weak, alias("Dummy_Handler")));
void I2S_Handler(void) __attribute__((weak, alias("Dummy_Handler")));
#endif

// CORTEX-M0+ handlers.
__attribute__((weak)) void
NMI_Handler (void)
{
    Dummy_Handler();
}
__attribute__((weak)) void
HardFault_Handler (void)
{
    Dummy_Handler();
}
__attribute__((weak)) void
SVC_Handler (void)
{
    Dummy_Handler();
}
__attribute__((weak)) void
PendSV_Handler (void)
{
    Dummy_Handler();
}
__attribute__((weak)) void
SysTick_Handler (void)
{
    Dummy_Handler();
}

// Peripheral handlers.
__attribute__((weak)) void
PM_Handler (void)
{
    Dummy_Handler();
}
__attribute__((weak)) void
SYSCTRL_Handler (void)
{
    Dummy_Handler();
}
__attribute__((weak)) void
WDT_Handler (void)
{
    Dummy_Handler();
}
__attribute__((weak)) void
RTC_Handler (void)
{
    Dummy_Handler();
}
__attribute__((weak)) void
EIC_Handler (void)
{
    Dummy_Handler();
}
__attribute__((weak)) void
NVMCTRL_Handler (void)
{
    Dummy_Handler();
}
__attribute__((weak)) void
DMAC_Handler (void)
{
    Dummy_Handler();
}
__attribute__((weak)) void
USB_Handler (void)
{
    Dummy_Handler();
}
__attribute__((weak)) void
EVSYS_Handler (void)
{
    Dummy_Handler();
}
__attribute__((weak)) void
SERCOM0_Handler (void)
{
    Dummy_Handler();
}
__attribute__((weak)) void
SERCOM1_Handler (void)
{
    Dummy_Handler();
}
__attribute__((weak)) void
SERCOM2_Handler (void)
{
    Dummy_Handler();
}
__attribute__((weak)) void
SERCOM3_Handler (void)
{
    Dummy_Handler();
}
__attribute__((weak)) void
SERCOM4_Handler (void)
{
    Dummy_Handler();
}
__attribute__((weak)) void
SERCOM5_Handler (void)
{
    Dummy_Handler();
}
__attribute__((weak)) void
TCC0_Handler (void)
{
    Dummy_Handler();
}
__attribute__((weak)) void
TCC1_Handler (void)
{
    Dummy_Handler();
}
__attribute__((weak)) void
TCC2_Handler (void)
{
    Dummy_Handler();
}
__attribute__((weak)) void
TC3_Handler (void)
{
    Dummy_Handler();
}
__attribute__((weak)) void
TC4_Handler (void)
{
    Dummy_Handler();
}
__attribute__((weak)) void
TC5_Handler (void)
{
    Dummy_Handler();
}
__attribute__((weak)) void
ADC_Handler (void)
{
    Dummy_Handler();
}
__attribute__((weak)) void
AC_Handler (void)
{
    Dummy_Handler();
}
__attribute__((weak)) void
DAC_Handler (void)
{
    Dummy_Handler();
}
__attribute__((weak)) void
PTC_Handler (void)
{
    Dummy_Handler();
}
__attribute__((weak)) void
I2S_Handler (void)
{
    Dummy_Handler();
}

// CORTEX-M0+ handlers.
__attribute__((section(".vectors"))) const DeviceVectors exception_table = {
    // Stack pointer.
    .pvStack = (void *)(&_estack),

    // CORTEX-M0+ handlers.
    .pfnReset_Handler     = (void *)Reset_Handler,
    .pfnNMI_Handler       = (void *)NMI_Handler,
    .pfnHardFault_Handler = (void *)HardFault_Handler,
    .pfnReservedM12       = (void *)(0UL),
    .pfnReservedM11       = (void *)(0UL),
    .pfnReservedM10       = (void *)(0UL),
    .pfnReservedM9        = (void *)(0UL),
    .pfnReservedM8        = (void *)(0UL),
    .pfnReservedM7        = (void *)(0UL),
    .pfnReservedM6        = (void *)(0UL),
    .pfnSVC_Handler       = (void *)SVC_Handler,
    .pfnReservedM4        = (void *)(0UL),
    .pfnReservedM3        = (void *)(0UL),
    .pfnPendSV_Handler    = (void *)PendSV_Handler,
    .pfnSysTick_Handler   = (void *)SysTick_Handler,

    // Peripheral handlers.
    .pfnPM_Handler      = (void *)PM_Handler,
    .pfnSYSCTRL_Handler = (void *)SYSCTRL_Handler,
    .pfnWDT_Handler     = (void *)WDT_Handler,
    .pfnRTC_Handler     = (void *)RTC_Handler,
    .pfnEIC_Handler     = (void *)EIC_Handler,
    .pfnNVMCTRL_Handler = (void *)NVMCTRL_Handler,
    .pfnDMAC_Handler    = (void *)DMAC_Handler,
    .pfnUSB_Handler     = (void *)USB_Handler,
    .pfnEVSYS_Handler   = (void *)EVSYS_Handler,
    .pfnSERCOM0_Handler = (void *)SERCOM0_Handler,
    .pfnSERCOM1_Handler = (void *)SERCOM1_Handler,
    .pfnSERCOM2_Handler = (void *)SERCOM2_Handler,
    .pfnSERCOM3_Handler = (void *)SERCOM3_Handler,
    .pfnSERCOM4_Handler = (void *)SERCOM4_Handler,
    .pfnSERCOM5_Handler = (void *)SERCOM5_Handler,
    .pfnTCC0_Handler    = (void *)TCC0_Handler,
    .pfnTCC1_Handler    = (void *)TCC1_Handler,
    .pfnTCC2_Handler    = (void *)TCC2_Handler,
    .pfnTC3_Handler     = (void *)TC3_Handler,
    .pfnTC4_Handler     = (void *)TC4_Handler,
    .pfnTC5_Handler     = (void *)TC5_Handler,
    .pfnReserved21      = (void *)(0UL),
    .pfnReserved22      = (void *)(0UL),
    .pfnADC_Handler     = (void *)ADC_Handler,
    .pfnAC_Handler      = (void *)AC_Handler,
    .pfnDAC_Handler     = (void *)DAC_Handler,
    .pfnPTC_Handler     = (void *)PTC_Handler,
    .pfnI2S_Handler     = (void *)I2S_Handler,
    .pfnReserved28      = (void *)(0UL),
};

void
Reset_Handler (void)
{
    uint32_t *src, *dest;

    // Initialize the relocate segment.
    src  = &_etext;
    dest = &_srelocate;

    if (src != dest)
    {
        for (; dest < &_erelocate;)
        {
            *dest++ = *src++;
        }
    }

    // Initialize the zero segment.
    for (dest = &_szero; dest < &_ezero;)
    {
        *dest++ = 0;
    }

    // Set the exception vector table base address.
    src       = (uint32_t *)&_sfixed;
    SCB->VTOR = ((uint32_t)&_sfixed & SCB_VTOR_TBLOFF_Msk);

    // Performance tuning for SRAM access.
    SBMATRIX->SFR[SBMATRIX_SLAVE_HMCRAMC0].reg = 2U;

    // Initialize the system.
    SystemInit();

    // Branch to main function.
    (void)main();

    for (;;)
    {
    }
}

void
Dummy_Handler (void)
{
    for (;;)
    {
    }
}
