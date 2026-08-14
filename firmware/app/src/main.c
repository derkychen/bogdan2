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
#include "board/indio/io_cfg.h"
#include "platform/samd21g18a/eic.h"
#include "platform/samd21g18a/time.h"
#include "platform/samd21g18a/usb.h"
#include <stdbool.h>
#include <string.h>

#define START_CMD                     ("{\"cmd\":\"start\"}")
#define START_POLL_INTERVAL_US        (100u)
#define START_TIMEOUT_MS              (5000u)
#define IO_CONFIGURE_POLL_INTERVAL_US (100u)
#define MAIN_LOOP_DELAY_US            (100u)

static controller_t x_controller;
static controller_t y_controller;
static relay_t      relay;
static profiler_t   profiler;

static parameters_t parameters = { 0 };
static char         message[SERIAL_READ_BUFFER_SIZE];

static void task(void);
static void init(void);
static bool poll_message(char const *expected,
                         uint32_t    poll_interval_us,
                         uint32_t    timeout_ms);

int
main (void)
{
    init();

    for (;;)
    {
        task();

        if (serial_read_line(message, sizeof(message))
            == SERIAL_STATUS_OK_LINE_RECEIVED)
        {
            if (parameters_parse_json(&parameters, message)
                == PARAMETERS_STATUS_OK_PARSED)
            {
                if (io_cfg_configure() != IO_CFG_STATUS_OK)
                {
                    serial_write_line(
                        "{\"ok\":false,\"msg\":\"io_configuration_failed\"}");
                }

                // Handshake to ensure movement occurs after host receives
                // status and completes required configuration.
                //
                // TODO: Check if this fixes the issue where the oscilloscope
                //       seems to not trigger on the last point. This is
                //       possibly an off-by-one error due to host configuration
                //       racing Trigger OUT from the first point.
                serial_write_line("{\"ok\":true,\"msg\":\"ready\"}");

                if (!poll_message("{\"cmd\":\"start\"}",
                                  START_POLL_INTERVAL_US,
                                  START_TIMEOUT_MS))
                {
                    serial_write_line(
                        "{\"ok\":false,\"msg\":\"start_timeout\"}");
                    continue;
                }

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

        time_sleep_us(MAIN_LOOP_DELAY_US);
    }
}

/** @brief Task function that should be called repeatedly in the `main` loop. */
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

    // Poll initialization of baseboard capabilities.
    while (io_cfg_configure() != IO_CFG_STATUS_OK)
    {
        usb_task();

        time_sleep_us(IO_CONFIGURE_POLL_INTERVAL_US);
    }

    // Initialize the beam profiler hardware. These functions configure the I/O
    // to safe defaults.
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

/** @brief Poll the serial connection for a message. */
static bool
poll_message (char const *expected,
              uint32_t    poll_interval_us,
              uint32_t    timeout_ms)
{
    uint32_t start_ms = time_get_ms();

    while (time_get_ms() - start_ms < timeout_ms)
    {
        task();

        if (serial_read_line(message, sizeof message)
                == SERIAL_STATUS_OK_LINE_RECEIVED
            && strcmp(message, expected) == 0)
        {
            return true;
        }

        time_sleep_us(poll_interval_us);
    }

    return false;
}
