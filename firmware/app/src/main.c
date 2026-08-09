/**
 * @file main.c
 * @brief Application program entry point.
 *
 * Implementation of the initialization of hardware and loop that polls the host
 * for beam profiling parameters.
 */
#include "app/controller.h"
#include "app/parameters.h"
#include "app/profiler.h"
#include "app/serial.h"
#include "board/indio/io_cfg.h"
#include "platform/samd21g18a/eic.h"
#include "platform/samd21g18a/time.h"
#include "platform/samd21g18a/usb.h"

#define MAIN_LOOP_DELAY_US (100u)

static controller_t x_controller;
static controller_t y_controller;
static relay_t      relay;
static profiler_t   profiler;
static parameters_t parameters = { 0 };
static char         message[SERIAL_READ_BUFFER_SIZE];

static void init(void);
static void task(void);

/**
 * @brief Beam profiler firmware application entry point.
 *
 * The microcontroller will poll the serial connection until it receives a set
 * of parameters. It will then control the traversal of the corresponding
 * grid. Once finished, it will resume polling for the next set of parameters.
 */
int
main (void)
{
    init();

    for (;;)
    {
        task();
        time_sleep_us(MAIN_LOOP_DELAY_US);

        if (serial_read_line(message, sizeof(message))
            == SERIAL_STATUS_OK_LINE_RECEIVED)
        {
            if (parameters_parse_json(&parameters, message)
                == PARAMETERS_STATUS_OK_PARSED)
            {
                serial_write_line(
                    "{\"ok\":true,\"msg\":\"parameters_received\"}");

                if (profiler_profile(&profiler, &parameters)
                    == PROFILER_STATUS_OK)
                {
                    serial_write_line("{\"ok\":true,\"msg\":\"profile_done\"}");
                }
                else
                {
                    serial_write_line(
                        "{\"ok\":false,\"msg\":\"profile_failed\"}");
                }
            }
            else
            {
                serial_write_line(
                    "{\"ok\":false,\"msg\":\"parameters_parse_failed\"}");
            }
        }
    }
}

/** @brief Initialize important functionality. */
static void
init (void)
{
    // NOTE: Platform initialization functions must be called before any other
    //       functionality.
    eic_init();
    evsys_init();
    time_init();
    usb_init();

    // Initialize baseboard capabilities
    io_cfg_init();

    // Initialize the beam profiler.
    //
    // NOTE: These functions configure the I/O to safe defaults.
    controller_init(&x_controller,
                    &io_cfg_expansion_d4_digital,
                    &io_cfg_expansion_d5_eic,
                    &io_cfg_analog_output_ch1);

    controller_init(&y_controller,
                    &io_cfg_expansion_d15_digital,
                    &io_cfg_expansion_d16_eic,
                    &io_cfg_analog_output_ch2);

    relay_init(&relay, &io_cfg_expansion_d7_pulser, &io_cfg_expansion_d6_eic);

    profiler_init(&profiler, &x_controller, &y_controller, &relay, task);

    // Initialize serial connection.
    serial_init();
}

/** @brief Task function that should be called repeatedly in the `main` loop. */
static void
task (void)
{
    usb_task();
}
