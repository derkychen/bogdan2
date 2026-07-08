#include "app/axis.h"
#include "app/controller.h"
#include "app/instruction.h"
#include "app/path.h"
#include "app/pulse_counter.h"
#include "app/serial.h"
#include "board/indio/io_cfg.h"
#include "platform/samd21g18a/assert.h"
#include "platform/samd21g18a/eic.h"
#include "platform/samd21g18a/time.h"
#include "platform/samd21g18a/usb.h"
#include <stdlib.h>

// TODO: Tighten delays after entire pipeline is tested.
#define USB_CONNECTION_POLL_INTERVAL_USEC (100U)
#define TARGET_SET_DEBOUNCE_TIME_USEC     (100U)
#define MAIN_LOOP_DELAY_USEC              (100U)
#define TIGHT_LOOP_DELAY_USEC             (100U)

/** @brief Initialize important functionality. */
static void
init (void)
{
    // NOTE: Platform initialization functions must be called before any other
    //       functionality.
    platform_samd21g18a_eic_init();
    platform_samd21g18a_time_init();
    platform_samd21g18a_usb_init();

    app_serial_init();
}

/** @brief Task function that should be called repeatedly in the `main` loop. */
static void
task (void)
{
    platform_samd21g18a_usb_task();
}

/**
 * @brief Bogdan 2 firmware application entry point.
 *
 * The Pico will poll the serial connection until it receives a set of
 * instructions. It will then control the traversal of the corresponding grid.
 * Once finished, it will resume polling for the next set of instructions.
 */
int
main (void)
{
    init();

    // Poll until a USB connection is established.
    while (!platform_samd21g18a_usb_is_mounted())
    {
        platform_samd21g18a_time_sleep_msec(USB_CONNECTION_POLL_INTERVAL_USEC);
    }

    app_serial_write_line("{\"ok\":true,\"msg\":\"waiting_for_instruction\"}");

    // These objects persist across profiles.
    app_instruction_t instruction;
    char              message[APP_SERIAL_READ_BUFFER_SIZE];

    app_controller_t x_controller;
    app_controller_t y_controller;

    app_controller_init(&x_controller,
                        &board_indio_expansion_d4_digital,
                        &board_indio_analog_output_ch1,
                        &board_indio_expansion_d7_eic,
                        &board_indio_analog_input_ch1);

    app_controller_init(&y_controller,
                        &board_indio_expansion_d5_digital,
                        &board_indio_analog_output_ch2,
                        &board_indio_expansion_d6_eic,
                        &board_indio_analog_input_ch2);

    app_pulse_counter_t pulse_counter;
    app_pulse_counter_init(&pulse_counter, &board_indio_expansion_d2_eic);

    app_path_raster_direction_t prev_raster_direction
        = APP_PATH_RASTER_DIRECTION_HORIZONTAL;

    for (;;)
    {
        if (app_serial_read_line(message, sizeof(message)))
        {
            if (app_instruction_parse_json(message, &instruction))
            {
                app_axis_t x;
                app_axis_t y;

                app_path_position_t *path;
                size_t               path_size;

                app_axis_init(&x,
                              instruction.x_min,
                              instruction.x_max,
                              instruction.x_unit_nm,
                              controller_read_analog_out(&x_controller),
                              &x_controller);

                app_axis_init(&y,
                              instruction.y_min,
                              instruction.y_max,
                              instruction.y_unit_nm,
                              controller_read_analog_out(&y_controller),
                              &y_controller);

                path = app_path_modified_raster(
                    &x, &y, &prev_raster_direction, &path_size);

                PLATFORM_SAMD21G18A_ASSERT(path != NULL);

                for (size_t i = 0; i < path_size; i++)
                {
                    app_axis_set_target(&x, path[i].x);
                    app_axis_set_target(&y, path[i].y);

                    platform_samd21g18a_time_sleep_usec(
                        TARGET_SET_DEBOUNCE_TIME_USEC);

                    // Move to the next point
                    app_axis_start_move(&x);
                    app_axis_start_move(&y);

                    while (app_axis_is_moving(&x) || app_axis_is_moving(&y))
                    {
                        task();
                    }

                    app_axis_move_end(&x);
                    app_axis_move_end(&y);

                    // Count pulses
                    while (app_pulse_counter_get(&pulse_counter)
                           < instruction.num_pulses)
                    {
                        task();
                    }

                    app_pulse_counter_reset(&pulse_counter);

                    platform_samd21g18a_time_sleep_usec(
                        instruction.posttrigger_time_us);
                }

                free(path);
                path = NULL;
            }
        }

        task();
    }
}
