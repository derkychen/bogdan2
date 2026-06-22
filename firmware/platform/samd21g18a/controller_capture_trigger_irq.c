/* Check which stage caused the interrupt */
#include "samd21g18a.h"
#include "controller.h"

extern Controller *controller_from_extint[NUM_EXTINT_LINES];

void EIC_Handler(void)
{
    // Create local copy of interrupt lines 
    uint32_t mask = CONTROLLER_TRIGGER_EXTINT_MASK;

    // Keep running as long as there are still EXTINT lines to check
    while (mask != 0)
    {
        // Counts trailing zeros to find EXTINT line
        // removes EXTINT line from mask
        // then checks next interrupt line at next loop
        uint8_t extint = __builtin_ctz(mask);
        mask &= ~(1u << extint);

        // If EXTINT line caused an interrupt
        if (EIC->INTFLAG.reg & (1u << extint))
        {
            // Clears hardware interrupt flag
            // prevents CPU from entering handler again for same interrupt
            EIC->INTFLAG.reg = (1u << extint);
            
            // Find which controller belong to that interrupt line
            Controller *controller = controller_from_extint[extint];

            if (controller != NULL)
            {
                controller->capture_requested = false;
            }
        }
    }
}
     
