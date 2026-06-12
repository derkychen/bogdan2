#include "pulse_counter.h"
#include "config.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "pico/types.h"
#include <stddef.h>

static PulseCounter *pulse_counter_for_irq = NULL;

/** @brief Increment counter upon interrupt from a laser pulse. */
static void pulse_counter_irq_handler(void) {
  uint32_t events = gpio_get_irq_event_mask(PULSE_TRIGGER_GPIO);

  if (events & GPIO_IRQ_EDGE_RISE) {
    gpio_acknowledge_irq(PULSE_TRIGGER_GPIO, GPIO_IRQ_EDGE_RISE);

    if (pulse_counter_for_irq != NULL) {
      pulse_counter_for_irq->count++;
    }
  }
}

PulseCounterInitStatusCode pulse_counter_init(PulseCounter *pulse_counter,
                                              uint trigger) {
  if (trigger != PULSE_TRIGGER_GPIO) {
    return PULSE_COUNTER_ERR_UNSUPPORTED_GPIO;
  }

  pulse_counter->trigger = trigger;
  pulse_counter->count = 0;

  pulse_counter_for_irq = pulse_counter;

  return PULSE_COUNTER_INIT_OK;
}

void pulse_counter_irq_init(void) {
  gpio_add_raw_irq_handler(PULSE_TRIGGER_GPIO, pulse_counter_irq_handler);
  gpio_set_irq_enabled(PULSE_TRIGGER_GPIO, GPIO_IRQ_EDGE_RISE, true);
  irq_set_enabled(IO_IRQ_BANK0, true);
}

void pulse_counter_reset(PulseCounter *pulse_counter) {
  pulse_counter->count = 0;
}

int pulse_counter_get(PulseCounter *pulse_counter) {
  return pulse_counter->count;
}
