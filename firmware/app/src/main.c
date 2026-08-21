/**
 * @file main.c
 * @brief Application program entry point.
 *
 * Performs initialization of hardware and loop that polls the host for beam
 * profiling parameters. The microcontroller controls movement and triggering
 * upon the reception of parameters.
 */
#include "app/controller.h"
#include "app/parameters.h"
#include "app/profiler.h"
#include "app/serial.h"
#include "board/indio/io.h"
#include "platform/samd21g18a/eic.h"
#include "platform/samd21g18a/time.h"
#include "platform/samd21g18a/usb.h"
#include <stdbool.h>
#include <stdio.h>

#define IO_CONFIGURE_POLL_INTERVAL_US (100u)
#define MAIN_LOOP_DELAY_US            (100u)

static controller_t x_controller;
static controller_t y_controller;
static relay_t      relay;
static profiler_t   profiler;

static char in_str[SERIAL_READ_BUF_SIZE];
static char out_str[512u];

static serial_buf_t in = {
    .str  = in_str,
    .size = sizeof(in_str),
};
static parameters_t parameters = { 0 };

static void task(void);
static void init(void);

int
main (void)
{
    init();

    for (;;)
    {
        task();

        if (serial_read_line(&in) == SERIAL_STATUS_OK_LINE_RECEIVED)
        {
            if (parameters_parse_json(&parameters, in.str)
                == PARAMETERS_STATUS_OK_PARSED)
            {
                if (io_configure() != IO_STATUS_OK)
                {
                    serial_write_line(
                        "{\"ok\":false,\"msg\":\"io_configuration_failed\"}");
                    continue;
                }

                serial_write_line("{\"ok\":true,\"msg\":\"ready\"}");

                profiler_status_t status
                    = profiler_profile(&profiler, &parameters);

                if (status == PROFILER_STATUS_OK)
                {
                    serial_write_line("{\"ok\":true,\"msg\":\"profile_done\"}");
                }
                else
                {
                    snprintf(out_str,
                             sizeof(out_str),
                             "{\"ok\":false,\"msg\":\"profile_failed %d\"}",
                             (int)status);

                    serial_write_line(out_str);
                }
            }
            else
            {
                serial_write_line(
                    "{\"ok\":false,\"msg\":\"parameters_parse_failed\"}");
            }
        }

        time_sleep_us(MAIN_LOOP_DELAY_US);
    }
}

/** @brief Task function that should be called in blocking loops. */
static void
task (void)
{
    usb_task();
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

    // Initialize baseboard capabilities.
    io_init();

    // Poll the configuration of baseboard I/O until ready.
    while (io_configure() != IO_STATUS_OK)
    {
        task();
        time_sleep_us(IO_CONFIGURE_POLL_INTERVAL_US);
    }

    // Initialize the beam profiler hardware.
    controller_init(&x_controller,
                    &io_expansion_d4_digital,
                    &io_expansion_d5_eic,
                    &io_analog_output_ch1);

    controller_init(&y_controller,
                    &io_expansion_d15_digital,
                    &io_expansion_d16_eic,
                    &io_analog_output_ch2);

    relay_init(&relay, &io_expansion_d7_pulser, &io_expansion_d6_eic);

    profiler_init(&profiler, &x_controller, &y_controller, &relay, &in, task);

    // Initialize serial connection.
    serial_init();
}
