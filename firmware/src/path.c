#include "path.h"
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
static int
get_anchor_coord (const Axis *axis)
{
    if (axis->min > 0)
    {
        return axis->min;
    }

    if (axis->max < 0)
    {
        return axis->max;
    }

    return 0;
}

/** @brief Append a point to a path. */
static bool
append (Position *path,
        size_t    path_size_capacity,
        size_t   *path_size_current,
        int       x,
        int       y)
{
    if (*path_size_current >= path_size_capacity)
    {
        return false;
    }

    path[*path_size_current].x = x;
    path[*path_size_current].y = y;
    (*path_size_current)++;

    return true;
}

/**
 * @brief Reverse the order of a slice of a path.
 *
 * This is used in the rotation of the path to start at any point on it. The
 * slice includes elements at both indices @p low and @p high.
 */
static void
reverse_path (Position *path, size_t low, size_t high)
{
    while (low < high)
    {
        Position tmp = path[low];
        path[low]    = path[high];
        path[high]   = tmp;

        low++;
        high--;
    }
}

/**
 * @brief Rotate a path so that the anchor point is first.
 *
 * Since the path is cyclic, its rotation preserves the traversal.
 */
static bool
rotate_to_anchor (Position *path, size_t path_size, int anchor_x, int anchor_y)
{
    // Search for the index of the anchor point.
    size_t anchor_index = 0;
    bool   found        = false;

    for (size_t i = 0; i < path_size; i++)
    {
        if (path[i].x == anchor_x && path[i].y == anchor_y)
        {
            anchor_index = i;
            found        = true;
            break;
        }
    }

    if (!found)
    {
        return false;
    }

    if (anchor_index == 0)
    {
        return true;
    }

    // Rotate the array.
    reverse_path(path, 0, anchor_index - 1);
    reverse_path(path, anchor_index, path_size - 1);
    reverse_path(path, 0, path_size - 1);

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
static RasterDirection
choose_raster_direction (size_t          x_num_points,
                         size_t          y_num_points,
                         RasterDirection prev_raster_direction)
{
    if ((x_num_points & 1U) == (y_num_points & 1U))
    {
        return (prev_raster_direction == RASTER_DIRECTION_HORIZONTAL)
                   ? RASTER_DIRECTION_VERTICAL
                   : RASTER_DIRECTION_HORIZONTAL;
    }

    return ((x_num_points & 1U) == 0U) ? RASTER_DIRECTION_VERTICAL
                                       : RASTER_DIRECTION_HORIZONTAL;
}

/**
 * @brief Transform and then append a local point to a path.
 *
 * The points on the path are initially generated in a local space to form a
 * horizontal modified raster in a coordinate system where each axis is indexed
 * from zero upward. This function transforms such a point to the actual grid,
 * which may involve translating along each axis and transposing (changing the
 * orientation of the modified raster).
 *
 * Working in the initial local space allows for both horizontal and vertical
 * modified rasters to utilize the same algorithm.
 */
static bool
append_local (Position   *path,
              size_t      path_size_capacity,
              size_t     *path_size_current,
              const Axis *x,
              const Axis *y,
              int         row,
              int         col,
              bool        transposed)
{
    if (transposed)
    {
        return append(path,
                      path_size_capacity,
                      path_size_current,
                      x->min + row,
                      y->min + col);
    }

    return append(path,
                  path_size_capacity,
                  path_size_current,
                  x->min + col,
                  y->min + row);
}

/**
 * @brief Generate a horizontal path for one-dimensional grids.
 *
 * The generated path starts and ends at the anchor, but skips over points as a
 * raster is impossible in this case.
 */
static bool
append_line (Position   *path,
             size_t      path_size_capacity,
             size_t     *path_size_current,
             const Axis *x,
             const Axis *y,
             int         num_points,
             int         anchor,
             bool        transposed)
{
    int col;

    if (num_points <= 0 || anchor < 0 || anchor >= num_points)
    {
        return false;
    }

    // Handle a single point.
    if (num_points == 1)
    {
        return append_local(path,
                            path_size_capacity,
                            path_size_current,
                            x,
                            y,
                            0,
                            0,
                            transposed);
    }

    // Traverse the line by going to the nearest end first, minimizing the
    // amount of time spent skipping over points before the profile is
    // completed.
    if (anchor <= (num_points - 1 - anchor))
    {
        for (col = anchor; col >= 0; col--)
        {
            if (!append_local(path,
                              path_size_capacity,
                              path_size_current,
                              x,
                              y,
                              0,
                              col,
                              transposed))
            {
                return false;
            }
        }

        for (col = anchor + 1; col < num_points; col++)
        {
            if (!append_local(path,
                              path_size_capacity,
                              path_size_current,
                              x,
                              y,
                              0,
                              col,
                              transposed))
            {
                return false;
            }
        }
    }
    else
    {
        for (col = anchor; col < num_points; col++)
        {
            if (!append_local(path,
                              path_size_capacity,
                              path_size_current,
                              x,
                              y,
                              0,
                              col,
                              transposed))
            {
                return false;
            }
        }

        for (col = anchor - 1; col >= 0; col--)
        {
            if (!append_local(path,
                              path_size_capacity,
                              path_size_current,
                              x,
                              y,
                              0,
                              col,
                              transposed))
            {
                return false;
            }
        }
    }

    return append_local(path,
                        path_size_capacity,
                        path_size_current,
                        x,
                        y,
                        0,
                        anchor,
                        transposed);
}

/**
 * @brief Generate a horizontal modified raster for an even grid.
 *
 * @p rows must be even.
 */
static bool
append_even_cycle (Position   *path,
                   size_t      path_size_capacity,
                   size_t     *path_size_current,
                   const Axis *x,
                   const Axis *y,
                   int         rows,
                   int         cols,
                   bool        transposed)
{
    int row;
    int col;

    // Ensure an even number of rows and a two-dimensional grid
    if ((rows & 1) != 0 || rows <= 1 || cols <= 1)
    {
        return false;
    }

    // Move to the top of the grid
    for (row = 0; row < rows; row++)
    {
        if (!append_local(path,
                          path_size_capacity,
                          path_size_current,
                          x,
                          y,
                          row,
                          0,
                          transposed))
        {
            return false;
        }
    }

    // Raster horizontally until the adjacent to the origin
    for (row = rows - 1; row >= 1; row--)
    {
        if (((rows - 1 - row) & 1) == 0)
        {
            for (col = 1; col < cols; col++)
            {
                if (!append_local(path,
                                  path_size_capacity,
                                  path_size_current,
                                  x,
                                  y,
                                  row,
                                  col,
                                  transposed))
                {
                    return false;
                }
            }
        }
        else
        {
            for (col = cols - 1; col >= 1; col--)
            {
                if (!append_local(path,
                                  path_size_capacity,
                                  path_size_current,
                                  x,
                                  y,
                                  row,
                                  col,
                                  transposed))
                {
                    return false;
                }
            }
        }
    }

    for (col = cols - 1; col >= 1; col--)
    {
        if (!append_local(path,
                          path_size_capacity,
                          path_size_current,
                          x,
                          y,
                          0,
                          col,
                          transposed))
        {
            return false;
        }
    }

    return true;
}

/**
 * @brief Generate a horizontal modified raster for an odd grid.
 *
 * @p rows and @p cols must both be odd.
 */
static bool
append_odd_cycle (Position   *path,
                  size_t      path_size_capacity,
                  size_t     *path_size_current,
                  const Axis *x,
                  const Axis *y,
                  int         rows,
                  int         cols,
                  bool        transposed)
{
    int row;
    int col;

    // Ensure an odd, two-dimensional grid.
    if ((rows & 1) == 0 || (cols & 1) == 0 || rows <= 1 || cols <= 1)
    {
        return false;
    }

    // Move to the top of the grid.
    for (row = 0; row < rows; row++)
    {
        if (!append_local(path,
                          path_size_capacity,
                          path_size_current,
                          x,
                          y,
                          row,
                          0,
                          transposed))
        {
            return false;
        }
    }

    // Raster horizontally until at the right with two rows left.
    for (col = 1; col < cols; col++)
    {
        if (!append_local(path,
                          path_size_capacity,
                          path_size_current,
                          x,
                          y,
                          rows - 1,
                          col,
                          transposed))
        {
            return false;
        }
    }

    for (row = rows - 2; row > 1; row--)
    {
        if (((rows - 2 - row) & 1) == 0)
        {
            for (col = cols - 1; col >= 1; col--)
            {
                if (!append_local(path,
                                  path_size_capacity,
                                  path_size_current,
                                  x,
                                  y,
                                  row,
                                  col,
                                  transposed))
                {
                    return false;
                }
            }
        }
        else
        {
            for (col = 1; col < cols; col++)
            {
                if (!append_local(path,
                                  path_size_capacity,
                                  path_size_current,
                                  x,
                                  y,
                                  row,
                                  col,
                                  transposed))
                {
                    return false;
                }
            }
        }
    }

    // Move diagonally at the bottom-right corner to ensure a cyclic path.
    if (!append_local(path,
                      path_size_capacity,
                      path_size_current,
                      x,
                      y,
                      1,
                      cols - 1,
                      transposed))
    {
        return false;
    }

    if (!append_local(path,
                      path_size_capacity,
                      path_size_current,
                      x,
                      y,
                      1,
                      cols - 2,
                      transposed))
    {
        return false;
    }

    if (!append_local(path,
                      path_size_capacity,
                      path_size_current,
                      x,
                      y,
                      0,
                      cols - 1,
                      transposed))
    {
        return false;
    }

    col = cols - 2;

    // Squiggle toward the left until adjacent to the origin.
    while (col >= 2)
    {
        if (!append_local(path,
                          path_size_capacity,
                          path_size_current,
                          x,
                          y,
                          0,
                          col,
                          transposed))
        {
            return false;
        }

        if (!append_local(path,
                          path_size_capacity,
                          path_size_current,
                          x,
                          y,
                          0,
                          col - 1,
                          transposed))
        {
            return false;
        }

        if (!append_local(path,
                          path_size_capacity,
                          path_size_current,
                          x,
                          y,
                          1,
                          col - 1,
                          transposed))
        {
            return false;
        }

        if (!append_local(path,
                          path_size_capacity,
                          path_size_current,
                          x,
                          y,
                          1,
                          col - 2,
                          transposed))
        {
            return false;
        }

        col -= 2;
    }

    return append_local(
        path, path_size_capacity, path_size_current, x, y, 0, 1, transposed);
}

Position *
modified_raster (const Axis      *x,
                 const Axis      *y,
                 RasterDirection *prev_raster_direction,
                 size_t          *path_size)
{
    size_t          x_num_points;
    size_t          y_num_points;
    size_t          grid_num_points;
    size_t          path_size_capacity;
    size_t          path_size_current = 0;
    int             rows;
    int             cols;
    int             anchor_x;
    int             anchor_y;
    bool            transposed;
    RasterDirection direction;
    Position       *path;

    if (path_size == NULL || x == NULL || y == NULL
        || prev_raster_direction == NULL)
    {
        return NULL;
    }

    *path_size = 0;

    x_num_points = axis_num_points(x);
    y_num_points = axis_num_points(y);

    if (x_num_points == 0 || y_num_points == 0)
    {
        return NULL;
    }

    if (x_num_points > INT_MAX || y_num_points > INT_MAX)
    {
        return NULL;
    }

    if (x_num_points > SIZE_MAX / y_num_points)
    {
        return NULL;
    }

    grid_num_points = x_num_points * y_num_points;

    if (grid_num_points > SIZE_MAX - 1)
    {
        return NULL;
    }

    path_size_capacity
        = (grid_num_points == 1) ? grid_num_points : grid_num_points + 1;

    if (path_size_capacity > SIZE_MAX / sizeof *path)
    {
        return NULL;
    }

    path = malloc(path_size_capacity * sizeof *path);

    if (path == NULL)
    {
        return NULL;
    }

    anchor_x = get_anchor_coord(x);
    anchor_y = get_anchor_coord(y);

    // Handle one dimensional grids.
    if (x_num_points == 1 || y_num_points == 1)
    {
        int num_points;
        int anchor;

        if (y_num_points == 1)
        {
            num_points = (int)x_num_points;
            anchor     = anchor_x - x->min;
            transposed = false;
        }
        else
        {
            num_points = (int)y_num_points;
            anchor     = anchor_y - y->min;
            transposed = true;
        }

        if (!append_line(path,
                         path_size_capacity,
                         &path_size_current,
                         x,
                         y,
                         num_points,
                         anchor,
                         transposed))
        {
            free(path);
            return NULL;
        }

        *path_size = path_size_current;
        return path;
    }

    // Handle two dimensional grid.
    direction = choose_raster_direction(
        x_num_points, y_num_points, *prev_raster_direction);
    *prev_raster_direction = direction;

    if (direction == RASTER_DIRECTION_HORIZONTAL)
    {
        rows       = (int)y_num_points;
        cols       = (int)x_num_points;
        transposed = false;
    }
    else
    {
        rows       = (int)x_num_points;
        cols       = (int)y_num_points;
        transposed = true;
    }

    if ((rows & 1) == 0)
    {
        if (!append_even_cycle(path,
                               path_size_capacity,
                               &path_size_current,
                               x,
                               y,
                               rows,
                               cols,
                               transposed))
        {
            free(path);
            return NULL;
        }
    }
    else if (((rows & 1) == 1) && ((cols & 1) == 1))
    {
        if (!append_odd_cycle(path,
                              path_size_capacity,
                              &path_size_current,
                              x,
                              y,
                              rows,
                              cols,
                              transposed))
        {
            free(path);
            return NULL;
        }
    }
    else
    {
        free(path);
        return NULL;
    }

    // Rotate the path to start at the anchor.
    if (path_size_current != grid_num_points
        || !rotate_to_anchor(path, grid_num_points, anchor_x, anchor_y))
    {
        free(path);
        return NULL;
    }

    path[grid_num_points] = path[0];
    *path_size            = path_size_capacity;

    return path;
}
