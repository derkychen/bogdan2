#ifndef APP_INSTRUCTION_H
#define APP_INSTRUCTION_H

#include <stdbool.h>
#include <stdint.h>

/** @brief Instruction status codes. */
typedef enum
{
    APP_INSTRUCTION_STATUS_OK_PARSED = 0,
    APP_INSTRUCTION_STATUS_ERR_JSON_MISSING_REQUIRED_FIELDS,
    APP_INSTRUCTION_STATUS_ERR_JSON_PARSE,
} app_instruction_status_t;

/**
 * @brief Instruction mode enumeration.
 *
 * `POINT_COUNT`: moves to each point on a grid and counts a number of pulses.
 * `POINT_TIME`:  moves to each point on a grid for a fixed amount of time.
 * `CONTINUOUS`:  moves across the entire grid continuously.
 */
typedef enum
{
    APP_INSTRUCTION_MODE_POINT_COUNT = 0,
    APP_INSTRUCTION_MODE_POINT_TIME,
    APP_INSTRUCTION_MODE_CONTINUOUS,

    APP_INSTRUCTION_MODE_COUNT,
} app_instruction_mode_t;

/**
 * @brief Microcontroller-specific instructions received from the host.
 *
 * This structure defines the grid to profile as well as some parameters
 * relevant to waveform capture.
 *
 * NOTE: This structure only contains the instructions relevant to the
 *       microcontroller. It is not necessarily the full set of instructions.
 *
 *       Depending on the mode, some of these are unnecessary.
 */
typedef struct
{
    /** Mode of the profiler. */
    app_instruction_mode_t mode;

    /** Minimum coordinate on the x-axis in units. */
    int x_min;

    /** Maximum coordinate on the x-axis in units. */
    int x_max;

    /** The length of each unit on the x-axis in nanometres. */
    uint32_t x_unit_nm;

    /** Position of the origin of x-axis after calibration in nanometres. */
    int x_origin_nm;

    /** Minimum coordinate on the x-axis in units. */
    int y_min;

    /** Maximum coordinate on the x-axis in units. */
    int y_max;

    /** The length of each unit on the x-axis in nanometres. */
    uint32_t y_unit_nm;

    /** Position of the origin of y-axis after calibration in nanometres. */
    int y_origin_nm;

    /** The number of laser pulses that should be captured at each point.*/
    uint32_t num_pulses;

    /** The amount of time to wait at each point, in microseconds. */
    uint32_t wait_time_us;

    /** The delay after the triggering of the PicoScope, in microseconds. */
    uint32_t posttrigger_time_us;
} app_instruction_t;

/** @brief Parse the JSON instruction sent through serial. */
app_instruction_status_t app_instruction_parse_json(app_instruction_t *inst,
                                                    char const        *json);

#endif
