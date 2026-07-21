#include "app/controller.h"
#include "app/instruction.h"
#include "app/profiler.h"
#include "app/pulse_receiver.h"
#include "app/serial.h"
#include "board/indio/io_cfg.h"
#include "platform/samd21g18a/eic.h"
#include "platform/samd21g18a/time.h"
#include "platform/samd21g18a/usb.h"

#define MAIN_LOOP_DELAY_USEC (100U)

/** @brief Initialize important functionality. */
static void
init (void)
{
    // NOTE: Platform initialization functions must be called before any other
    //       functionality.
    platform_samd21g18a_eic_init();
    platform_samd21g18a_time_init();
    platform_samd21g18a_usb_init();

    // Initialize I/O to safe states and defaults.
    board_indio_io_cfg_init();

    // Initialize serial.
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
    // These objects persist across profiles.
    app_instruction_t instruction;
    char              message[APP_SERIAL_READ_BUFFER_SIZE];

    app_controller_t     x_controller;
    app_controller_t     y_controller;
    app_pulse_receiver_t receiver;
    app_profiler_t       profiler;

    init();

    app_controller_init(&x_controller,
                        &board_indio_io_cfg_expansion_d4_digital,
                        &board_indio_io_cfg_expansion_d5_eic,
                        &board_indio_io_cfg_analog_output_ch1);

    app_controller_init(&y_controller,
                        &board_indio_io_cfg_expansion_d15_digital,
                        &board_indio_io_cfg_expansion_d16_eic,
                        &board_indio_io_cfg_analog_output_ch2);

    app_pulse_receiver_init(&receiver,
                            &board_indio_io_cfg_expansion_d6_eic,
                            &board_indio_io_cfg_expansion_d7_digital);

    if (app_profiler_init(
            &profiler, &x_controller, &y_controller, &receiver, task)
        != APP_PROFILER_STATUS_OK)
    {
        app_serial_write_line(
            "{\"ok\":false,\"msg\":\"profiler_init_failed\"}");
    }

    for (;;)
    {
        task();
        platform_samd21g18a_time_sleep_usec(MAIN_LOOP_DELAY_USEC);

        if (app_serial_read_line(message, sizeof(message))
            == APP_SERIAL_STATUS_OK_LINE_RECEIVED)
        {
            if (app_instruction_parse_json(&instruction, message)
                == APP_INSTRUCTION_STATUS_OK_PARSED)
            {
                app_serial_write_line(
                    "{\"ok\":true,\"msg\":\"instructions_received\"}");

                if (app_profiler_profile(&profiler, &instruction)
                    == APP_PROFILER_STATUS_OK)
                {
                    app_serial_write_line(
                        "{\"ok\":true,\"msg\":\"profile_complete\"}");
                }
                else
                {
                    app_serial_write_line(
                        "{\"ok\":false,\"msg\":\"profile_failed\"}");
                }
            }
            else
            {
                app_serial_write_line(
                    "{\"ok\":false,\"msg\":\"instruction_parse_failed\"}");
            }
        }
    }
}
