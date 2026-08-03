#ifndef APP_PATH_H
#define APP_PATH_H

#include "app/axis.h"
#include <stdbool.h>
#include <stddef.h>

/** @brief Define a point with x and y coordinates. */
typedef struct
{
    /** The x-coordinate in units. */
    int x;

    /** The y-coordinate in units. */
    int y;
} path_position_t;

/** @brief Path status codes. */
typedef enum
{
    PATH_STATUS_OK = 0,
    PATH_STATUS_ERR,
} path_status_t;

/**
 * @brief For storage of the direction of the raster.
 *
 * The raster direction is parallel to the axis along which most travel occurs.
 * Thus, a horizontal raster resembles the letter Z, and a vertical raster
 * resembles the letter N.
 *
 * NOTE: This is important in the actual geometry of the path as well as the
 *       balancing of consecutive travel between the two stages.
 */
typedef enum
{
    PATH_RASTER_DIRECTION_HORIZONTAL = 0,
    PATH_RASTER_DIRECTION_VERTICAL,
} path_raster_direction_t;

/**
 * @brief Generate an array of Position structures that form a modified raster.
 *
 * The modified raster is a raster scan that starts and ends at the origin or
 * the point closest to it. It alternates between a horizontal and vertical
 * raster for grids where both dimensions are even or odd. One diagonal movement
 * is used for odd grids, as it takes the same amount of time and a Hamiltonian
 * cycle is not possible.
 *
 * This function also handles edge cases (e.g. grids of insufficient size, axes
 * that do not contain the origin, etc.). This function helps minimize the
 * movement of the stages for each grid and balance consecutive travel between
 * the stages.
 */
path_status_t path_modified_raster(
    axis_t const            *x,
    axis_t const            *y,
    path_raster_direction_t *prev_raster_direction,
    bool                     corners_only,
    path_position_t        **path,
    size_t                  *path_size);

#endif
