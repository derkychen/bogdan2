#include "axis.h"
#include "config.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/pwm.h"
#include "pico/time.h"
#include <pico/types.h>

#define AXIS_IRQ_GPIO_MASK                                                     \
  ((1u << X_TRIGGER_OUT_GPIO) | (1u << Y_TRIGGER_OUT_GPIO))
#define ANALOG_MIN_VAL 0
#define ANALOG_MAX_VAL 65535
#define START_MOVE_PULSE_WIDTH_US 10

static Axis *axis_from_trigger_out[NUM_GPIO] = {0};

static void axes_trigger_out_irq_handler(void) {
  uint32_t mask = AXIS_IRQ_GPIO_MASK;

  while (mask != 0) {
    uint gpio = __builtin_ctz(mask);
    mask &= ~(1u << gpio);

    Axis *axis = axis_from_trigger_out[gpio];

    if (axis == NULL) {
      continue;
    }

    uint32_t events = gpio_get_irq_event_mask(gpio);

    if (events & GPIO_IRQ_EDGE_RISE) {
      gpio_acknowledge_irq(gpio, GPIO_IRQ_EDGE_RISE);
      axis->moving = false;
    }
  }
}

AxisInitStatusCode axis_init(Axis *axis, int min, int max, int unit_nm,
                             int trigger_in, int analog_in, int trigger_out,
                             int cur) {
  if (axis == NULL) {
    return AXIS_INIT_ERR_NULL;
  }

  if (trigger_out < 0 || trigger_out >= NUM_BANK0_GPIOS) {
    return AXIS_INIT_ERR_INVALID_GPIO;
  }

  if ((trigger_out != X_TRIGGER_OUT_GPIO) &&
      (trigger_out != Y_TRIGGER_OUT_GPIO)) {
    return AXIS_INIT_ERR_UNSUPPORTED_GPIO;
  }

  if (axis_from_trigger_out[trigger_out] != NULL) {
    return AXIS_INIT_ERR_DUPLICATE_TRIGGER_OUT;
  }

  axis->min = min;
  axis->max = max;
  axis->unit_nm = unit_nm;

  axis->trigger_in = trigger_in;
  axis->analog_in = analog_in;
  axis->trigger_out = trigger_out;

  axis->cur = cur;
  axis->target = axis->cur;
  axis->moving = false;

  axis_from_trigger_out[trigger_out] = axis;

  return AXIS_INIT_OK;
}

void axis_irq_init(void) {
  gpio_add_raw_irq_handler_with_order_priority_masked(
      AXIS_IRQ_GPIO_MASK, axes_trigger_out_irq_handler,
      PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY);

  gpio_set_irq_enabled(X_TRIGGER_OUT_GPIO, GPIO_IRQ_EDGE_RISE, true);
  gpio_set_irq_enabled(Y_TRIGGER_OUT_GPIO, GPIO_IRQ_EDGE_RISE, true);

  irq_set_enabled(IO_IRQ_BANK0, true);
}

int axis_num_points(Axis *axis) { return axis->max - axis->min + 1; }

// TODO: Check if PWM jitter causes controller to think target is changing. Will
//       need DAC or low-pass filter maybe.
void axis_set_target(Axis *axis, int target) {

  axis->target = target;

  // Do nothing if the stage is at the target
  if (axis->cur == axis->target) {
    return;
  }

  // Analog value of the coordinate
  int analog_val = (target - axis->min) * (ANALOG_MAX_VAL - ANALOG_MIN_VAL) /
                       (axis->max - axis->min) +
                   ANALOG_MIN_VAL;

  // Write the analog value to the controller Analog IN using PWM
  int slice = pwm_gpio_to_slice_num(axis->analog_in);
  int channel = pwm_gpio_to_channel(axis->analog_in);

  pwm_set_chan_level(slice, channel, analog_val);
}

void axis_start_move(Axis *axis) {

  // Do nothing if the stage is at the target
  if (axis->cur == axis->target) {
    return;
  }

  // Set state variable, this value will be changed upon an interrupt
  axis->moving = true;

  // Send a voltage pulse to the controller Trigger IN
  gpio_put(axis->trigger_in, 1);
  sleep_us(START_MOVE_PULSE_WIDTH_US);
  gpio_put(axis->trigger_in, 0);
}

void axis_move_end(Axis *axis) {

  // Do nothing if the stage did not move
  if (axis->cur == axis->target) {
    return;
  }

  axis->cur = axis->target;
}
