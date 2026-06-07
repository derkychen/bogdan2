#include "path.h"
#include "axis.h"
#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>

/**
 * @brief Get the coordinate of the path anchor on an axis.
 *
 * This function returns zero, or the coordinate nearest to zero on the axis.
 * The point on the grid whose coordinates are closest to zero is referred to as
 * the anchor, and it is where the traversal of the grid begins and ends.
 */
static int get_anchor_coord(const Axis *axis) {
  if (axis->min > 0) {
    return axis->min;
  }

  if (axis->max < 0) {
    return axis->max;
  }

  return 0;
}

/** @brief Append a point to a path. */
static bool append(Position *path, int capacity, int *count, int x, int y) {
  if (*count >= capacity) {
    return false;
  }

  path[*count].x = x;
  path[*count].y = y;
  (*count)++;

  return true;
}

/**
 * @brief Reverse the order of a slice of a path.
 *
 * This is used in the rotation of the path to start at any point on it. The
 * slice includes elements at both indices @p low and @p high.
 */
static void reverse_path(Position *path, int low, int high) {
  while (low < high) {
    Position tmp = path[low];
    path[low] = path[high];
    path[high] = tmp;

    low++;
    high--;
  }
}

/**
 * @brief Rotate a path so that the anchor point is first.
 *
 * Since the path is cyclic, its rotation preserves the traversal.
 */
static bool rotate_to_anchor(Position *path, int count, int anchor_x,
                             int anchor_y) {
  int i;
  int anchor_index = -1;

  // Search for the index of the anchor point
  for (i = 0; i < count; i++) {
    if (path[i].x == anchor_x && path[i].y == anchor_y) {
      anchor_index = i;
      break;
    }
  }

  if (anchor_index < 0) {
    return false;
  }

  if (anchor_index == 0) {
    return true;
  }

  // Rotate the array
  reverse_path(path, 0, anchor_index - 1);
  reverse_path(path, anchor_index, count - 1);
  reverse_path(path, 0, count - 1);

  return true;
}

/**
 * @brief Choose the raster direction.
 *
 * If the dimensions of the grid are both even or both odd, either raster
 * direction can be chosen, so the program chooses the opposite of the direction
 * that was last used. Otherwise, the raster direction must be parallel to the
 * odd side.
 */
static RasterDirection choose_raster_direction(int x_num_points,
                                               int y_num_points,
                                               RasterDirection prev) {
  if ((x_num_points & 1) == (y_num_points & 1)) {
    return (prev == RASTER_HORIZONTAL) ? RASTER_VERTICAL : RASTER_HORIZONTAL;
  }

  return ((y_num_points & 1) == 0) ? RASTER_VERTICAL : RASTER_HORIZONTAL;
}

/**
 * @brief Transform and append a zeroed point to a path.
 *
 * This function transforms a point in the zero-based coordinate system in which
 * the raster is generated to the coordinate system defined by the grid. Working
 * in the zero-based coordinate system allows for both horizontal and vertical
 * rasters to utilise the same algorithm.
 */
static bool append_zeroed(Position *path, int capacity, int *count,
                          const Axis *x, const Axis *y, int row, int col,
                          bool transposed) {
  if (transposed) {
    return append(path, capacity, count, x->min + row, y->min + col);
  }

  return append(path, capacity, count, x->min + col, y->min + row);
}

/**
 * @brief Generate a horizontal path for one-dimensional grids.
 *
 * The generated path starts and ends at the anchor, but skips over points as a
 * raster is impossible in this case.
 */
static bool append_line(Position *path, int capacity, int *count, const Axis *x,
                        const Axis *y, int num_points, int anchor,
                        bool transposed) {
  int col;

  // Single point
  if (num_points == 1) {
    return append_zeroed(path, capacity, count, x, y, 0, 0, transposed);
  }

  // Traverse the line by going to the nearest end first, minimizing the amount
  // of time spent skipping over points before the profile is completed
  if (anchor <= (num_points - 1 - anchor)) {
    for (col = anchor; col >= 0; col--) {
      if (!append_zeroed(path, capacity, count, x, y, 0, col, transposed)) {
        return false;
      }
    }

    for (col = anchor + 1; col < num_points; col++) {
      if (!append_zeroed(path, capacity, count, x, y, 0, col, transposed)) {
        return false;
      }
    }
  } else {
    for (col = anchor; col < num_points; col++) {
      if (!append_zeroed(path, capacity, count, x, y, 0, col, transposed)) {
        return false;
      }
    }

    for (col = anchor - 1; col >= 0; col--) {
      if (!append_zeroed(path, capacity, count, x, y, 0, col, transposed)) {
        return false;
      }
    }
  }

  return append_zeroed(path, capacity, count, x, y, 0, anchor, transposed);
}

/**
 * @brief Generate a horizontal raster for an even grid.
 *
 * @p rows must be even.
 */
static bool append_even_cycle(Position *path, int capacity, int *count,
                              const Axis *x, const Axis *y, int rows, int cols,
                              bool transposed) {
  int row;
  int col;

  // Ensure an even number of rows and a two-dimensional grid
  if ((rows & 1) != 0 || rows <= 1 || cols <= 1) {
    return false;
  }

  // Move to the top of the grid
  for (row = 0; row < rows; row++) {
    if (!append_zeroed(path, capacity, count, x, y, row, 0, transposed)) {
      return false;
    }
  }

  // Raster horizontally until the adjacent to the origin
  for (row = rows - 1; row >= 1; row--) {
    if (((rows - 1 - row) & 1) == 0) {
      for (col = 1; col < cols; col++) {
        if (!append_zeroed(path, capacity, count, x, y, row, col, transposed)) {
          return false;
        }
      }
    } else {
      for (col = cols - 1; col >= 1; col--) {
        if (!append_zeroed(path, capacity, count, x, y, row, col, transposed)) {
          return false;
        }
      }
    }
  }

  for (col = cols - 1; col >= 1; col--) {
    if (!append_zeroed(path, capacity, count, x, y, 0, col, transposed)) {
      return false;
    }
  }

  return true;
}

/**
 * @brief Generate a zeroed horizontal raster for an odd grid.
 *
 * @p rows and @p cols must both be odd.
 */
static bool append_odd_cycle(Position *path, int capacity, int *count,
                             const Axis *x, const Axis *y, int rows, int cols,
                             bool transposed) {
  int row;
  int col;

  // Ensure an odd, two-dimensional grid
  if ((rows & 1) == 0 || (cols & 1) == 0 || rows <= 1 || cols <= 1) {
    return false;
  }

  // Move to the top of the grid
  for (row = 0; row < rows; row++) {
    if (!append_zeroed(path, capacity, count, x, y, row, 0, transposed)) {
      return false;
    }
  }

  // Raster horizontally until at the right with two rows left
  for (col = 1; col < cols; col++) {
    if (!append_zeroed(path, capacity, count, x, y, rows - 1, col,
                       transposed)) {
      return false;
    }
  }

  for (row = rows - 2; row > 1; row--) {
    if (((rows - 2 - row) & 1) == 0) {
      for (col = cols - 1; col >= 1; col--) {
        if (!append_zeroed(path, capacity, count, x, y, row, col, transposed)) {
          return false;
        }
      }
    } else {
      for (col = 1; col < cols; col++) {
        if (!append_zeroed(path, capacity, count, x, y, row, col, transposed)) {
          return false;
        }
      }
    }
  }

  // Move diagonally at the bottom-right corner to ensure a cyclic path
  if (!append_zeroed(path, capacity, count, x, y, 1, cols - 1, transposed)) {
    return false;
  }

  if (!append_zeroed(path, capacity, count, x, y, 1, cols - 2, transposed)) {
    return false;
  }

  if (!append_zeroed(path, capacity, count, x, y, 0, cols - 1, transposed)) {
    return false;
  }

  col = cols - 2;

  // Squiggle toward the left until adjacent to the origin
  while (col >= 2) {
    if (!append_zeroed(path, capacity, count, x, y, 0, col, transposed)) {
      return false;
    }

    if (!append_zeroed(path, capacity, count, x, y, 0, col - 1, transposed)) {
      return false;
    }

    if (!append_zeroed(path, capacity, count, x, y, 1, col - 1, transposed)) {
      return false;
    }

    if (!append_zeroed(path, capacity, count, x, y, 1, col - 2, transposed)) {
      return false;
    }

    col -= 2;
  }

  return append_zeroed(path, capacity, count, x, y, 0, 1, transposed);
}

Position *modified_raster(Axis *x, Axis *y, RasterDirection prev,
                          int *path_size) {
  int x_num_points;
  int y_num_points;
  int total;
  int capacity;
  int count = 0;
  int rows;
  int cols;
  int anchor_x;
  int anchor_y;
  bool transposed;
  RasterDirection direction;
  Position *path;

  if (path_size == NULL || x == NULL || y == NULL) {
    return NULL;
  }

  *path_size = 0;

  if (x->min > x->max || y->min > y->max) {
    return NULL;
  }

  x_num_points = axis_num_points(x);
  y_num_points = axis_num_points(y);

  if (x_num_points <= 0 || y_num_points <= 0 ||
      x_num_points > INT_MAX / y_num_points) {
    return NULL;
  }

  total = x_num_points * y_num_points;

  if (total == INT_MAX) {
    return NULL;
  }

  capacity = total + 1;

  if ((size_t)capacity > ((size_t)-1) / sizeof(*path)) {
    return NULL;
  }

  path = (Position *)malloc((size_t)capacity * sizeof(*path));
  if (path == NULL) {
    return NULL;
  }

  anchor_x = get_anchor_coord(x);
  anchor_y = get_anchor_coord(y);

  // Handle one dimensional grid
  if (x_num_points == 1 || y_num_points == 1) {
    int num_points;
    int anchor;

    if (y_num_points == 1) {
      num_points = x_num_points;
      anchor = anchor_x - x->min;
      transposed = false;
    } else {
      num_points = y_num_points;
      anchor = anchor_y - y->min;
      transposed = true;
    }

    if (!append_line(path, capacity, &count, x, y, num_points, anchor,
                     transposed)) {
      free(path);
      return NULL;
    }

    *path_size = count;
    return path;
  }

  // Handle two dimensional grid
  direction = choose_raster_direction(x_num_points, y_num_points, prev);

  if (direction == RASTER_VERTICAL) {
    rows = y_num_points;
    cols = x_num_points;
    transposed = false;
  } else {
    rows = x_num_points;
    cols = y_num_points;
    transposed = true;
  }

  if ((rows & 1) == 0) {
    if (!append_even_cycle(path, capacity, &count, x, y, rows, cols,
                           transposed)) {
      free(path);
      return NULL;
    }
  } else if (((rows & 1) == 1) && ((cols & 1) == 1)) {
    if (!append_odd_cycle(path, capacity, &count, x, y, rows, cols,
                          transposed)) {
      free(path);
      return NULL;
    }
  } else {
    free(path);
    return NULL;
  }

  // Rotate the path to start at the anchor
  if (count != total || !rotate_to_anchor(path, total, anchor_x, anchor_y)) {
    free(path);
    return NULL;
  }

  path[total] = path[0];
  *path_size = capacity;

  return path;
}
