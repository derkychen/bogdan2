#include "app/path.h"
#include "app/axis.h"
#include "platform/samd21g18a/assert.h"
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

/**
 * @brief Get the coordinate of the path anchor on an axis.
 *
 * This function returns zero, or the coordinate nearest to zero on the axis.
 * The point on the grid whose coordinates are closest to zero is referred to as
 * the anchor, and it is where the traversal of the grid begins and ends.
 */
static int
get_anchor_coord (app_axis_t const *axis)
{
    PLATFORM_SAMD21G18A_ASSERT(axis != NULL);

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
static void
append (app_path_position_t *path,
        size_t               path_size_capacity,
        size_t              *path_size_current,
        int                  x,
        int                  y)
{
    PLATFORM_SAMD21G18A_ASSERT(path != NULL);
    PLATFORM_SAMD21G18A_ASSERT(path_size_current != NULL);
    PLATFORM_SAMD21G18A_ASSERT(*path_size_current < path_size_capacity);

    path[*path_size_current].x = x;
    path[*path_size_current].y = y;
    (*path_size_current)++;

    return;
}

/**
 * @brief Reverse the order of a slice of a path.
 *
 * This is used in the rotation of the path to start at any point on it. The
 * slice includes elements at both indices @p low and @p high.
 */
static void
reverse_path (app_path_position_t *path, size_t low, size_t high)
{
    PLATFORM_SAMD21G18A_ASSERT(path != NULL);

    while (low < high)
    {
        app_path_position_t tmp = path[low];
        path[low]               = path[high];
        path[high]              = tmp;

        low++;
        high--;
    }

    return;
}

/**
 * @brief Rotate a path so that the anchor point is first.
 *
 * Since the path is cyclic, its rotation preserves the traversal.
 */
static void
rotate_to_anchor (app_path_position_t *path,
                  size_t               path_size,
                  int                  anchor_x,
                  int                  anchor_y)
{
    // Search for the index of the anchor point.
    size_t anchor_index = 0;
    bool   found        = false;

    PLATFORM_SAMD21G18A_ASSERT(path != NULL);

    for (size_t i = 0; i < path_size; i++)
    {
        if (path[i].x == anchor_x && path[i].y == anchor_y)
        {
            anchor_index = i;
            found        = true;
            break;
        }
    }

    PLATFORM_SAMD21G18A_ASSERT(found);

    if (anchor_index == 0)
    {
        return;
    }

    // Rotate the array.
    reverse_path(path, 0, anchor_index - 1);
    reverse_path(path, anchor_index, path_size - 1);
    reverse_path(path, 0, path_size - 1);

    return;
}

/**
 * @brief Choose the raster direction.
 *
 * If the dimensions of the grid are both even or both odd, either raster
 * direction can be chosen, so the program chooses the opposite of the direction
 * that was last used. Otherwise, the raster direction must be parallel to the
 * odd side.
 */
static app_path_raster_direction_t
choose_raster_direction (size_t                      x_num_points,
                         size_t                      y_num_points,
                         app_path_raster_direction_t prev_raster_direction)
{
    if ((x_num_points & 1u) == (y_num_points & 1u))
    {
        return (prev_raster_direction == APP_PATH_RASTER_DIRECTION_HORIZONTAL)
                   ? APP_PATH_RASTER_DIRECTION_VERTICAL
                   : APP_PATH_RASTER_DIRECTION_HORIZONTAL;
    }

    return ((x_num_points & 1u) == 0u) ? APP_PATH_RASTER_DIRECTION_VERTICAL
                                       : APP_PATH_RASTER_DIRECTION_HORIZONTAL;
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
static void
append_local (app_path_position_t *path,
              size_t               path_size_capacity,
              size_t              *path_size_current,
              app_axis_t const    *x,
              app_axis_t const    *y,
              int                  row,
              int                  col,
              bool                 transposed)
{
    PLATFORM_SAMD21G18A_ASSERT(path != NULL);
    PLATFORM_SAMD21G18A_ASSERT(path_size_current != NULL);
    PLATFORM_SAMD21G18A_ASSERT(x != NULL);
    PLATFORM_SAMD21G18A_ASSERT(y != NULL);

    if (transposed)
    {
        append(path,
               path_size_capacity,
               path_size_current,
               x->min + row,
               y->min + col);

        return;
    }

    append(path,
           path_size_capacity,
           path_size_current,
           x->min + col,
           y->min + row);

    return;
}

/**
 * @brief Generate a horizontal path for one-dimensional grids.
 *
 * The generated path starts and ends at the anchor, but skips over points as a
 * raster is impossible in this case.
 */
static void
append_line (app_path_position_t *path,
             size_t               path_size_capacity,
             size_t              *path_size_current,
             app_axis_t const    *x,
             app_axis_t const    *y,
             int                  num_points,
             int                  anchor,
             bool                 transposed)
{
    int col;

    PLATFORM_SAMD21G18A_ASSERT(path != NULL);
    PLATFORM_SAMD21G18A_ASSERT(path_size_current != NULL);
    PLATFORM_SAMD21G18A_ASSERT(x != NULL);
    PLATFORM_SAMD21G18A_ASSERT(y != NULL);

    PLATFORM_SAMD21G18A_ASSERT(num_points > 0);
    PLATFORM_SAMD21G18A_ASSERT(anchor >= 0);
    PLATFORM_SAMD21G18A_ASSERT(anchor < num_points);

    // Handle a single point.
    if (num_points == 1)
    {
        append_local(path,
                     path_size_capacity,
                     path_size_current,
                     x,
                     y,
                     0,
                     0,
                     transposed);

        return;
    }

    // Traverse the line by going to the nearest end first, minimizing the
    // amount of time spent skipping over points before the profile is
    // completed.
    if (anchor <= (num_points - 1 - anchor))
    {
        for (col = anchor; col >= 0; col--)
        {
            append_local(path,
                         path_size_capacity,
                         path_size_current,
                         x,
                         y,
                         0,
                         col,
                         transposed);
        }

        for (col = anchor + 1; col < num_points; col++)
        {
            append_local(path,
                         path_size_capacity,
                         path_size_current,
                         x,
                         y,
                         0,
                         col,
                         transposed);
        }
    }
    else
    {
        for (col = anchor; col < num_points; col++)
        {
            append_local(path,
                         path_size_capacity,
                         path_size_current,
                         x,
                         y,
                         0,
                         col,
                         transposed);
        }

        for (col = anchor - 1; col >= 0; col--)
        {
            append_local(path,
                         path_size_capacity,
                         path_size_current,
                         x,
                         y,
                         0,
                         col,
                         transposed);
        }
    }

    append_local(path,
                 path_size_capacity,
                 path_size_current,
                 x,
                 y,
                 0,
                 anchor,
                 transposed);

    return;
}

/**
 * @brief Generate a horizontal modified raster for an even grid.
 *
 * @p rows must be even.
 */
static void
append_even_unrotated_path (app_path_position_t *path,
                            size_t               path_size_capacity,
                            size_t              *path_size_current,
                            app_axis_t const    *x,
                            app_axis_t const    *y,
                            int                  rows,
                            int                  cols,
                            bool                 transposed)
{
    int row;
    int col;

    PLATFORM_SAMD21G18A_ASSERT(path != NULL);
    PLATFORM_SAMD21G18A_ASSERT(path_size_current != NULL);
    PLATFORM_SAMD21G18A_ASSERT(x != NULL);
    PLATFORM_SAMD21G18A_ASSERT(y != NULL);

    // Ensure an even number of rows and a two-dimensional grid.
    PLATFORM_SAMD21G18A_ASSERT((rows & 1) == 0);
    PLATFORM_SAMD21G18A_ASSERT(rows > 1);
    PLATFORM_SAMD21G18A_ASSERT(cols > 1);

    // Move to the top of the grid.
    for (row = 0; row < rows; row++)
    {
        append_local(path,
                     path_size_capacity,
                     path_size_current,
                     x,
                     y,
                     row,
                     0,
                     transposed);
    }

    // Raster horizontally until the adjacent to the origin.
    for (row = rows - 1; row >= 1; row--)
    {
        if (((rows - 1 - row) & 1) == 0)
        {
            for (col = 1; col < cols; col++)
            {
                append_local(path,
                             path_size_capacity,
                             path_size_current,
                             x,
                             y,
                             row,
                             col,
                             transposed);
            }
        }
        else
        {
            for (col = cols - 1; col >= 1; col--)
            {
                append_local(path,
                             path_size_capacity,
                             path_size_current,
                             x,
                             y,
                             row,
                             col,
                             transposed);
            }
        }
    }

    for (col = cols - 1; col >= 1; col--)
    {
        append_local(path,
                     path_size_capacity,
                     path_size_current,
                     x,
                     y,
                     0,
                     col,
                     transposed);
    }

    return;
}

/**
 * @brief Generate a horizontal modified raster for an odd grid.
 *
 * @p rows and @p cols must both be odd.
 */
static void
append_odd_unrotated_path (app_path_position_t *path,
                           size_t               path_size_capacity,
                           size_t              *path_size_current,
                           app_axis_t const    *x,
                           app_axis_t const    *y,
                           int                  rows,
                           int                  cols,
                           bool                 transposed)
{
    int row;
    int col;

    PLATFORM_SAMD21G18A_ASSERT(path != NULL);
    PLATFORM_SAMD21G18A_ASSERT(path_size_current != NULL);
    PLATFORM_SAMD21G18A_ASSERT(x != NULL);
    PLATFORM_SAMD21G18A_ASSERT(y != NULL);

    // Ensure an odd, two-dimensional grid.
    PLATFORM_SAMD21G18A_ASSERT((rows & 1) != 0);
    PLATFORM_SAMD21G18A_ASSERT((cols & 1) != 0);
    PLATFORM_SAMD21G18A_ASSERT(rows > 1);
    PLATFORM_SAMD21G18A_ASSERT(cols > 1);

    // Move to the top of the grid.
    for (row = 0; row < rows; row++)
    {
        append_local(path,
                     path_size_capacity,
                     path_size_current,
                     x,
                     y,
                     row,
                     0,
                     transposed);
    }

    // Raster horizontally until at the right with two rows left.
    for (col = 1; col < cols; col++)
    {
        append_local(path,
                     path_size_capacity,
                     path_size_current,
                     x,
                     y,
                     rows - 1,
                     col,
                     transposed);
    }

    for (row = rows - 2; row > 1; row--)
    {
        if (((rows - 2 - row) & 1) == 0)
        {
            for (col = cols - 1; col >= 1; col--)
            {
                append_local(path,
                             path_size_capacity,
                             path_size_current,
                             x,
                             y,
                             row,
                             col,
                             transposed);
            }
        }
        else
        {
            for (col = 1; col < cols; col++)
            {
                append_local(path,
                             path_size_capacity,
                             path_size_current,
                             x,
                             y,
                             row,
                             col,
                             transposed);
            }
        }
    }

    // Move diagonally at the bottom-right corner to ensure a cyclic path.
    append_local(path,
                 path_size_capacity,
                 path_size_current,
                 x,
                 y,
                 1,
                 cols - 1,
                 transposed);

    append_local(path,
                 path_size_capacity,
                 path_size_current,
                 x,
                 y,
                 1,
                 cols - 2,
                 transposed);

    append_local(path,
                 path_size_capacity,
                 path_size_current,
                 x,
                 y,
                 0,
                 cols - 1,
                 transposed);

    col = cols - 2;

    // Squiggle toward the left until adjacent to the origin.
    while (col >= 2)
    {
        append_local(path,
                     path_size_capacity,
                     path_size_current,
                     x,
                     y,
                     0,
                     col,
                     transposed);

        append_local(path,
                     path_size_capacity,
                     path_size_current,
                     x,
                     y,
                     0,
                     col - 1,
                     transposed);

        append_local(path,
                     path_size_capacity,
                     path_size_current,
                     x,
                     y,
                     1,
                     col - 1,
                     transposed);

        append_local(path,
                     path_size_capacity,
                     path_size_current,
                     x,
                     y,
                     1,
                     col - 2,
                     transposed);

        col -= 2;
    }

    append_local(
        path, path_size_capacity, path_size_current, x, y, 0, 1, transposed);

    return;
}

/** @brief Determine whether a path changes direction at a point. */
static bool
corner_at (const app_path_position_t *previous,
           const app_path_position_t *current,
           const app_path_position_t *next)
{
    int64_t dx_previous;
    int64_t dy_previous;
    int64_t dx_next;
    int64_t dy_next;
    int64_t cross_product;
    int64_t dot_product;

    PLATFORM_SAMD21G18A_ASSERT(previous != NULL);
    PLATFORM_SAMD21G18A_ASSERT(current != NULL);
    PLATFORM_SAMD21G18A_ASSERT(next != NULL);

    dx_previous = (int64_t)current->x - previous->x;
    dy_previous = (int64_t)current->y - previous->y;
    dx_next     = (int64_t)next->x - current->x;
    dy_next     = (int64_t)next->y - current->y;

    cross_product = (dx_previous * dy_next) - (dy_previous * dx_next);
    dot_product   = (dx_previous * dx_next) + (dy_previous * dy_next);

    return cross_product != 0 || dot_product <= 0;
}

/** @brief Remove points that do not represent corners in a path. */
static void
shrink_to_corners (app_path_position_t *path, size_t *path_size_current)
{
    size_t write_index;

    PLATFORM_SAMD21G18A_ASSERT(path != NULL);
    PLATFORM_SAMD21G18A_ASSERT(path_size_current != NULL);

    if (*path_size_current <= 2)
    {
        return;
    }

    write_index = 1;

    for (size_t read_index = 1; read_index + 1 < *path_size_current;
         read_index++)
    {
        if (corner_at(&path[read_index - 1],
                      &path[read_index],
                      &path[read_index + 1]))
        {
            path[write_index] = path[read_index];
            write_index++;
        }
    }

    path[write_index] = path[*path_size_current - 1];
    write_index++;

    *path_size_current = write_index;

    return;
}

app_path_position_t *
app_path_modified_raster (app_axis_t const            *x,
                          app_axis_t const            *y,
                          app_path_raster_direction_t *prev_raster_direction,
                          bool                         corners_only,
                          size_t                      *path_size)
{
    size_t                      x_num_points;
    size_t                      y_num_points;
    size_t                      grid_num_points;
    size_t                      path_size_capacity;
    int                         rows;
    int                         cols;
    int                         anchor_x;
    int                         anchor_y;
    bool                        transposed;
    app_path_raster_direction_t direction;
    app_path_position_t        *path;

    PLATFORM_SAMD21G18A_ASSERT(x != NULL);
    PLATFORM_SAMD21G18A_ASSERT(y != NULL);
    PLATFORM_SAMD21G18A_ASSERT(prev_raster_direction != NULL);
    PLATFORM_SAMD21G18A_ASSERT(path_size != NULL);

    *path_size = 0;

    x_num_points = app_axis_num_points(x);
    y_num_points = app_axis_num_points(y);

    PLATFORM_SAMD21G18A_ASSERT(x_num_points > 0);
    PLATFORM_SAMD21G18A_ASSERT(y_num_points > 0);
    PLATFORM_SAMD21G18A_ASSERT(x_num_points <= INT_MAX);
    PLATFORM_SAMD21G18A_ASSERT(y_num_points <= INT_MAX);
    PLATFORM_SAMD21G18A_ASSERT(x_num_points <= (SIZE_MAX / y_num_points));

    grid_num_points = x_num_points * y_num_points;

    PLATFORM_SAMD21G18A_ASSERT(grid_num_points <= (SIZE_MAX - 1));

    path_size_capacity
        = (grid_num_points == 1) ? grid_num_points : grid_num_points + 1;

    PLATFORM_SAMD21G18A_ASSERT(path_size_capacity <= (SIZE_MAX / sizeof *path));

    path = malloc(path_size_capacity * sizeof *path);

    // NOTE: The only failure mode for path generation should be a failed memory
    //       allocation.
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

        append_line(path,
                    path_size_capacity,
                    path_size,
                    x,
                    y,
                    num_points,
                    anchor,
                    transposed);

        if (corners_only)
        {
            shrink_to_corners(path, path_size);
        }

        return path;
    }

    // Handle two dimensional grid.
    direction = choose_raster_direction(
        x_num_points, y_num_points, *prev_raster_direction);
    *prev_raster_direction = direction;

    if (direction == APP_PATH_RASTER_DIRECTION_HORIZONTAL)
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
        append_even_unrotated_path(
            path, path_size_capacity, path_size, x, y, rows, cols, transposed);
    }
    else if (((rows & 1) == 1) && ((cols & 1) == 1))
    {
        append_odd_unrotated_path(
            path, path_size_capacity, path_size, x, y, rows, cols, transposed);
    }
    else
    {
        PLATFORM_SAMD21G18A_ASSERT(false);
    }

    PLATFORM_SAMD21G18A_ASSERT(*path_size == grid_num_points);

    // Rotate the path to start at the anchor.
    rotate_to_anchor(path, grid_num_points, anchor_x, anchor_y);

    path[grid_num_points] = path[0];
    *path_size            = path_size_capacity;

    if (corners_only)
    {
        shrink_to_corners(path, path_size);
    }

    return path;
}
