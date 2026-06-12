#include "controller.h"
#include "config.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"
#include "pico/time.h"
#include "serial.h"

#define CONTROLLER_IRQ_GPIO_MASK \
    ((1u << X_TRIGGER_OUT_GPIO) | (1u << Y_TRIGGER_OUT_GPIO))
#define START_MOVE_PULSE_WIDTH_US 10

static Controller *controller_from_trigger_out[NUM_BANK0_GPIOS] = { 0 };

/**
 * @brief Record the stopping of stage movement upon an interrupt for the
 *        corresponding controller.
 */
static void
controllers_trigger_out_irq_handler (void)
{
    uint32_t mask = CONTROLLER_IRQ_GPIO_MASK;

    while (mask != 0)
    {
        uint gpio = __builtin_ctz(mask);
        mask &= ~(1u << gpio);

        Controller *controller = controller_from_trigger_out[gpio];

        if (controller == NULL)
        {
            continue;
        }

        uint32_t events = gpio_get_irq_event_mask(gpio);

        if (events & GPIO_IRQ_EDGE_RISE)
        {
            gpio_acknowledge_irq(gpio, GPIO_IRQ_EDGE_RISE);
            controller->stage_moving = false;
        }
    }
}

void
controller_init (Controller *controller,
                 uint        trigger_in,
                 uint        analog_in,
                 uint        trigger_out,
                 uint        analog_out_channel)
{
    if ((trigger_out != X_TRIGGER_OUT_GPIO)
        && (trigger_out != Y_TRIGGER_OUT_GPIO))
    {
        serial_print_error(
            "Attempted to initialize controller on unsupported GPIO pin.");
    }

    if (controller_from_trigger_out[trigger_out] != NULL)
    {
        serial_print_error(
            "Attempted to initialize controller occupied GPIO pin.");
    }

    controller->trigger_in         = trigger_in;
    controller->analog_in          = analog_in;
    controller->trigger_out        = trigger_out;
    controller->analog_out_channel = analog_out_channel;

    controller->stage_moving = false;

    // Add this controller to the GPIO to controller map.
    controller_from_trigger_out[trigger_out] = controller;
}

void
controllers_irq_init (void)
{
    gpio_add_raw_irq_handler_with_order_priority_masked(
        CONTROLLER_IRQ_GPIO_MASK,
        controllers_trigger_out_irq_handler,
        PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY);

    // Configure both controller Trigger OUT interrupts on rising edges.
    gpio_set_irq_enabled(X_TRIGGER_OUT_GPIO, GPIO_IRQ_EDGE_RISE, true);
    gpio_set_irq_enabled(Y_TRIGGER_OUT_GPIO, GPIO_IRQ_EDGE_RISE, true);

    irq_set_enabled(IO_IRQ_BANK0, true);
}

void
controller_set_stage_moving (Controller *controller)
{
    // NOTE: This value will be changed to `false` upon an interrupt.
    controller->stage_moving = true;
}

void
controller_pulse_trigger_in (const Controller *controller)
{
    // Send a voltage pulse to the controller Trigger IN
    gpio_put(controller->trigger_in, 1);
    sleep_us(START_MOVE_PULSE_WIDTH_US);
    gpio_put(controller->trigger_in, 0);
}

// TODO: Check if PWM jitter causes controller to think target is changing. Will
//       need DAC or low-pass filter maybe.
void
controller_write_analog_in (const Controller *controller, uint16_t analog_val)
{
    // Write the analog value to the controller Analog IN using PWM
    int slice   = pwm_gpio_to_slice_num(controller->analog_in);
    int channel = pwm_gpio_to_channel(controller->analog_in);

    pwm_set_chan_level(slice, channel, analog_val);
}

uint16_t
controller_read_analog_out (const Controller *controller)
{
    adc_select_input(controller->analog_out_channel);

    return adc_read();
}

bool
controller_is_stage_moving (const Controller *controller)
{
    return controller->stage_moving;
}
