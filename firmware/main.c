#include "axis.h"
#include "config.h"
#include "instruction.h"
#include "path.h"
#include "pico/stdio.h"
#include "pico/stdio_usb.h"
#include "pico/time.h"
#include "pulse_counter.h"
#include "serial.h"
#include <stdlib.h>

#define USB_CONNECTION_POLL_INTERVAL_MS 10
#define MAIN_LOOP_POLL_INTERVAL_MS 10
#define TARGET_SET_BUFFER_TIME_US 10

/**
 * @brief Bogdan 2 Pico program entry point.
 *
 * The Pico will poll the serial connection until it receives a set of
 * instructions. It will then control the traversal of the corresponding grid.
 * Once finished, it will resume polling for the next set of instructions.
 *
 * Some data persist throughout each profile, such as the positions of each
 * stage, as well as the previous raster direction. These data are cleared when
 * Pico is reset,
 */
int main(void) {
  config_init();
  stdio_init_all();

  // Poll until USB connection
  while (!stdio_usb_connected()) {
    sleep_ms(USB_CONNECTION_POLL_INTERVAL_MS);
  }

  serial_print_waiting();

  // These variables persist across profiles
  Instruction inst;
  char message[SERIAL_LINE_BUFFER_SIZE];

  int x_cur = 0;
  int y_cur = 0;

  RasterDirection prev_raster_direction = RASTER_DIRECTION_HORIZONTAL;

  // Main loop
  for (;;) {
    if (serial_read_line(message, sizeof(message))) {
      if (instruction_parse_json(message, &inst)) {
        Axis x;
        Axis y;

        Position *path;
        int path_size;

        PulseCounter pulse_counter;

        path = modified_raster(&x, &y, &prev_raster_direction, &path_size);

        if (path == NULL) {
          serial_print_error("A path could not be generated; this is likely a "
                             "problem in the code.");
        }

        // TODO: Add status code checking
        axis_init(&x, inst.x_min, inst.x_max, inst.x_unit_nm, X_TRIGGER_IN_GPIO,
                  X_ANALOG_IN_GPIO, X_TRIGGER_OUT_GPIO, x_cur);
        axis_init(&x, inst.x_min, inst.x_max, inst.x_unit_nm, X_TRIGGER_IN_GPIO,
                  X_ANALOG_IN_GPIO, X_TRIGGER_OUT_GPIO, y_cur);

        pulse_counter_init(&pulse_counter, PULSE_TRIGGER_GPIO);

        for (int i = 0; i < path_size; i++) {
          axis_set_target(&x, path[i].x);
          axis_set_target(&y, path[i].y);

          sleep_us(TARGET_SET_BUFFER_TIME_US);

          axis_start_move(&x);
          axis_start_move(&y);

          while (x.moving || y.moving) {
            tight_loop_contents();
          }

          axis_move_end(&x);
          axis_move_end(&y);

          while (pulse_counter.count < inst.num_pulses) {
            tight_loop_contents();
          }

          sleep_us(inst.posttrigger_time_us);
        }

        free(path);
        path = NULL;

        x_cur = x.cur;
        y_cur = y.cur;
      }
    }

    sleep_ms(MAIN_LOOP_POLL_INTERVAL_MS);
  }
}
