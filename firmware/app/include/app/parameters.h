/**
 * @file parameters.h
 * @brief Parameters of the beam profiler.
 *
 * The purpose of this module is to provide a function that parses a JSON sent
 * from the host into a set of parameters according to which the beam profiler
 * will move.
 */
#ifndef APP_PARAMETERS_H
#define APP_PARAMETERS_H

#include <stdbool.h>
#include <stdint.h>

/** @brief Parameters status codes. */
typedef enum
{
    PARAMETERS_STATUS_OK_PARSED = 0,
    PARAMETERS_STATUS_ERR_JSON_DUPLICATE_FIELD,
    PARAMETERS_STATUS_ERR_JSON_FIELD_NOT_ALLOWED,
    PARAMETERS_STATUS_ERR_JSON_MISSING_REQUIRED_FIELDS,
    PARAMETERS_STATUS_ERR_JSON_PARSE,
    PARAMETERS_STATUS_ERR_JSON_UNKNOWN_FIELD,
} parameters_status_t;

/**
 * @brief Parameters mode enumeration.
 *
 * `POINT_COUNT`: moves to each point on a grid and counts a number of pulses.
 * `POINT_TIME`:  moves to each point on a grid for a fixed amount of time.
 * `CONTINUOUS`:  moves across the entire grid continuously.
 */
typedef enum
{
    PARAMETERS_MODE_POINT_COUNT = 0,
    PARAMETERS_MODE_POINT_TIME,
    PARAMETERS_MODE_CONTINUOUS,

    PARAMETERS_MODE_COUNT,
} parameters_mode_t;

/**
 * @brief Microcontroller-specific parameters received from the host.
 *
 * This structure defines the grid to profile as well as some parameters
 * relevant to waveform capture.
 *
 * NOTE: This structure only contains the parameters relevant to the
 *       microcontroller. It is not necessarily the full set of parameters.
 *
 *       Depending on the mode, some of these are unnecessary.
 */
typedef struct
{
    parameters_mode_t mode; /**< Mode of the profiler. */

    int      x_min;     /**< Minimum coordinate on the x-axis in units. */
    int      x_max;     /**< Maximum coordinate on the x-axis in units. */
    uint32_t x_unit_nm; /**< Unit length on the x-axis in nanometres. */
    int x_origin_nm;    /**< Position of the origin of x-axis in nanometres. */

    int      y_min;     /**< Minimum coordinate on the y-axis in units. */
    int      y_max;     /**< Maximum coordinate on the y-axis in units. */
    uint32_t y_unit_nm; /**< Unit length on the y-axis in nanometres. */
    int y_origin_nm;    /**< Position of the origin of y-axis in nanometres. */

    uint32_t num_pulses;   /**< Number of laser pulses to capture per point. */
    uint32_t wait_time_us; /**< Time to wait at each point, in microseconds. */

    uint32_t posttrigger_time_us; /**< Time after trigger, in microseconds. */
} parameters_t;

/** @brief Parse the JSON parameters sent through serial. */
parameters_status_t parameters_parse_json(parameters_t *parameters,
                                          char const   *json);

#endif
