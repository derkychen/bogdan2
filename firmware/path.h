#ifndef PATH_H
#define PATH_H

#include "axis.h"

/** @brief Define a point with x and y coordinates. */
typedef struct Position {

  /** The x-coordinate in units. */
  int x;

  /** The y-coordinate in units. */
  int y;

} Position;

/**
 * @brief For storage of the direction of the raster.
 *
 * This is important in the actual geometry of the path as well as the balancing
 * of consecutive travel between the two stages.
 */
typedef enum RasterDirection {

  /** Raster direction parallel to the x-axis (like the letter Z). */
  RASTER_HORIZONTAL = 0,

  /** Raster direction parallel to the y-axis (like the letter N). */
  RASTER_VERTICAL = 1,

} RasterDirection;

/**
 * @brief Generate an array of Position structures that form a modified raster.
 *
 * The modified raster performs a raster scan that starts and ends at the origin
 * or the point closest to it. It alternates between a horizontal and vertical
 * raster for grids where both dimensions are even or odd. One diagonal movement
 * is used for odd grids, as it takes the same amount of time and a Hamiltonian
 * cycle is not possible.
 *
 * This function also handles edge cases (e.g. grids of insufficient size, axes
 * that do not contain the origin, etc.). This function helps minimize the
 * movement of the stages for each grid and balance consecutive travel between
 * the stages.
 */
Position *modified_raster(Axis *x, Axis *y, RasterDirection *prev,
                          int *path_size);

#endif
