/**
 * @file path.c
 * @brief Implementation of the generation of a modified raster.
 *
 * NOTE: The static allocation `path_buffer` occupies the majority of all of the
 *       SRAM available on the processor. As such, if modifications to the rest
 *       of the program cause errors upon compilation regarding insufficient
 *       SRAM, decreasing the size of the buffer is likely to be a solution.
 */
#include "app/path.h"
#include "app/axis.h"
#include "platform/samd21g18a/assert.h"
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>

#define PATH_BUFFER_CAPACITY (2048u)

static path_position_t path_buffer[PATH_BUFFER_CAPACITY];

static int  get_anchor_coord(axis_t const *axis);
static void append(path_position_t *path,
                   size_t          *path_size,
                   size_t           path_size_capacity,
                   int              x,
                   int              y);
static void reverse_path(path_position_t *path, size_t low, size_t high);
static void rotate_to_anchor(path_position_t *path,
                             size_t           path_size,
                             int              anchor_x,
                             int              anchor_y);
static path_raster_direction_t choose_raster_direction(
    size_t                  x_num_points,
    size_t                  y_num_points,
    path_raster_direction_t prev_raster_direction);
static void append_local(path_position_t *path,
                         size_t          *path_size,
                         size_t           path_size_capacity,
                         axis_t const    *x,
                         axis_t const    *y,
                         int              row,
                         int              col,
                         bool             transposed);
static void append_line(path_position_t *path,
                        size_t          *path_size,
                        size_t           path_size_capacity,
                        axis_t const    *x,
                        axis_t const    *y,
                        int              num_points,
                        int              anchor,
                        bool             transposed);
static void append_even_unrotated_path(path_position_t *path,
                                       size_t          *path_size,
                                       size_t           path_size_capacity,
                                       axis_t const    *x,
                                       axis_t const    *y,
                                       int              rows,
                                       int              cols,
                                       bool             transposed);
static void append_odd_unrotated_path(path_position_t *path,
                                      size_t          *path_size,
                                      size_t           path_size_capacity,
                                      axis_t const    *x,
                                      axis_t const    *y,
                                      int              rows,
                                      int              cols,
                                      bool             transposed);
static bool corner_at(path_position_t const *prev,
                      path_position_t const *curr,
                      path_position_t const *next);
static void shrink_to_corners(path_position_t *path, size_t *path_size);

path_status_t
path_modified_raster (axis_t const            *x,
                      axis_t const            *y,
                      path_raster_direction_t *prev_raster_direction,
                      bool                     corners_only,
                      path_position_t        **path,
                      size_t                  *path_size)
{
    ASSERT(x != NULL);
    ASSERT(y != NULL);
    ASSERT(prev_raster_direction != NULL);
    ASSERT(path_size != NULL);

    *path_size = 0u;

    size_t x_num_points = axis_num_points(x);
    size_t y_num_points = axis_num_points(y);

    ASSERT(x_num_points > 0u);
    ASSERT(x_num_points <= INT_MAX);
    ASSERT(y_num_points > 0u);
    ASSERT(y_num_points <= INT_MAX);
    ASSERT(x_num_points <= (SIZE_MAX / y_num_points));

    size_t grid_num_points = x_num_points * y_num_points;

    if (grid_num_points == 0)
    {
        return PATH_STATUS_ERR;
    }

    size_t path_buffer_capacity = sizeof path_buffer / sizeof path_buffer[0];
    size_t closing_point_count  = (grid_num_points > 1u) ? 1u : 0u;

    if (path_buffer_capacity < (grid_num_points + closing_point_count))
    {
        return PATH_STATUS_ERR;
    }

    size_t path_size_capacity = grid_num_points + closing_point_count;

    int anchor_x = get_anchor_coord(x);
    int anchor_y = get_anchor_coord(y);

    bool transposed;

    // Handle one dimensional grids.
    if (x_num_points == 1u || y_num_points == 1u)
    {
        int num_points;
        int anchor;

        if (y_num_points == 1u)
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

        append_line(path_buffer,
                    path_size,
                    path_size_capacity,
                    x,
                    y,
                    num_points,
                    anchor,
                    transposed);

        if (corners_only)
        {
            shrink_to_corners(path_buffer, path_size);
        }

        *path = path_buffer;

        return PATH_STATUS_OK;
    }

    // Handle two dimensional grid.
    path_raster_direction_t direction = choose_raster_direction(
        x_num_points, y_num_points, *prev_raster_direction);
    *prev_raster_direction = direction;

    int rows;
    int cols;

    if (direction == PATH_RASTER_DIRECTION_HORIZONTAL)
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
        append_even_unrotated_path(path_buffer,
                                   path_size,
                                   path_size_capacity,
                                   x,
                                   y,
                                   rows,
                                   cols,
                                   transposed);
    }
    else if (((rows & 1) == 1) && ((cols & 1) == 1))
    {
        append_odd_unrotated_path(path_buffer,
                                  path_size,
                                  path_size_capacity,
                                  x,
                                  y,
                                  rows,
                                  cols,
                                  transposed);
    }
    else
    {
        ASSERT(false);
    }

    ASSERT(*path_size == grid_num_points);

    // Rotate the path to start at the anchor.
    rotate_to_anchor(path_buffer, grid_num_points, anchor_x, anchor_y);

    path_buffer[grid_num_points] = path_buffer[0];
    *path_size                   = path_size_capacity;

    if (corners_only)
    {
        shrink_to_corners(path_buffer, path_size);
    }

    *path = path_buffer;

    return PATH_STATUS_OK;
}

/**
 * @brief Get the coordinate of the path anchor on an axis.
 *
 * This function returns zero, or the coordinate nearest to zero on the axis.
 * The point on the grid whose coordinates are closest to zero is referred to as
 * the anchor, and it is where the traversal of the grid begins and ends.
 */
static int
get_anchor_coord (axis_t const *axis)
{
    ASSERT(axis != NULL);

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
append (path_position_t *path,
        size_t          *path_size,
        size_t           path_size_capacity,
        int              x,
        int              y)
{
    ASSERT(path != NULL);
    ASSERT(path_size != NULL);
    ASSERT(*path_size < path_size_capacity);

    path[*path_size].x = x;
    path[*path_size].y = y;
    (*path_size)++;

    return;
}

/**
 * @brief Reverse the order of a slice of a path.
 *
 * This is used in the rotation of the path to start at any point on it. The
 * slice includes elements at both indices @p low and @p high.
 */
static void
reverse_path (path_position_t *path, size_t low, size_t high)
{
    ASSERT(path != NULL);

    while (low < high)
    {
        path_position_t tmp = path[low];
        path[low]           = path[high];
        path[high]          = tmp;

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
rotate_to_anchor (path_position_t *path,
                  size_t           path_size,
                  int              anchor_x,
                  int              anchor_y)
{
    // Search for the index of the anchor point.
    size_t anchor_index = 0u;
    bool   found        = false;

    ASSERT(path != NULL);

    for (size_t i = 0u; i < path_size; i++)
    {
        if (path[i].x == anchor_x && path[i].y == anchor_y)
        {
            anchor_index = i;
            found        = true;
            break;
        }
    }

    ASSERT(found);

    if (anchor_index == 0u)
    {
        return;
    }

    // Rotate the array.
    reverse_path(path, 0u, anchor_index - 1u);
    reverse_path(path, anchor_index, path_size - 1u);
    reverse_path(path, 0u, path_size - 1u);

    return;
}

/**
 * @brief Choose the raster direction.
 *
 * If the dimensions of the grid are both even or both odd, either raster
 * direction can be chosen, so the program chooses the opposite of the direction
 * that was last used. Otherwise, the raster direction must be parallel to the
 * odd side.
 *
 * NOTE: The direction chosen ensures that the number of local rows is even.
 */
static path_raster_direction_t
choose_raster_direction (size_t                  x_num_points,
                         size_t                  y_num_points,
                         path_raster_direction_t prev_raster_direction)
{
    if ((x_num_points & 1u) == (y_num_points & 1u))
    {
        return (prev_raster_direction == PATH_RASTER_DIRECTION_HORIZONTAL)
                   ? PATH_RASTER_DIRECTION_VERTICAL
                   : PATH_RASTER_DIRECTION_HORIZONTAL;
    }

    return ((x_num_points & 1u) == 0u) ? PATH_RASTER_DIRECTION_VERTICAL
                                       : PATH_RASTER_DIRECTION_HORIZONTAL;
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
append_local (path_position_t *path,
              size_t          *path_size,
              size_t           path_size_capacity,
              axis_t const    *x,
              axis_t const    *y,
              int              row,
              int              col,
              bool             transposed)
{
    ASSERT(path != NULL);
    ASSERT(path_size != NULL);
    ASSERT(x != NULL);
    ASSERT(y != NULL);

    if (transposed)
    {
        append(path, path_size, path_size_capacity, x->min + row, y->min + col);

        return;
    }

    append(path, path_size, path_size_capacity, x->min + col, y->min + row);

    return;
}

/**
 * @brief Generate a horizontal path for one-dimensional grids.
 *
 * The generated path starts and ends at the anchor, but skips over points as a
 * raster is impossible in this case.
 */
static void
append_line (path_position_t *path,
             size_t          *path_size,
             size_t           path_size_capacity,
             axis_t const    *x,
             axis_t const    *y,
             int              num_points,
             int              anchor,
             bool             transposed)
{
    ASSERT(path != NULL);
    ASSERT(path_size != NULL);
    ASSERT(x != NULL);
    ASSERT(y != NULL);

    ASSERT(num_points > 0);
    ASSERT(anchor >= 0);
    ASSERT(anchor < num_points);

    // Handle a single point.
    if (num_points == 1)
    {
        append_local(
            path, path_size, path_size_capacity, x, y, 0, 0, transposed);

        return;
    }

    // Traverse the line by going to the nearest end first, minimizing the
    // amount of time spent skipping over points before the profile is
    // completed.
    int col;

    if (anchor <= (num_points - 1 - anchor))
    {
        for (col = anchor; col >= 0; col--)
        {
            append_local(
                path, path_size, path_size_capacity, x, y, 0, col, transposed);
        }

        for (col = anchor + 1; col < num_points; col++)
        {
            append_local(
                path, path_size, path_size_capacity, x, y, 0, col, transposed);
        }
    }
    else
    {
        for (col = anchor; col < num_points; col++)
        {
            append_local(
                path, path_size, path_size_capacity, x, y, 0, col, transposed);
        }

        for (col = anchor - 1; col >= 0; col--)
        {
            append_local(
                path, path_size, path_size_capacity, x, y, 0, col, transposed);
        }
    }

    append_local(
        path, path_size, path_size_capacity, x, y, 0, anchor, transposed);

    return;
}

/**
 * @brief Generate a horizontal modified raster for an even grid.
 *
 * @p rows must be even.
 */
static void
append_even_unrotated_path (path_position_t *path,
                            size_t          *path_size,
                            size_t           path_size_capacity,
                            axis_t const    *x,
                            axis_t const    *y,
                            int              rows,
                            int              cols,
                            bool             transposed)
{
    ASSERT(path != NULL);
    ASSERT(path_size != NULL);
    ASSERT(x != NULL);
    ASSERT(y != NULL);

    // Ensure an even number of rows and a two-dimensional grid.
    ASSERT((rows & 1) == 0);
    ASSERT(rows > 1);
    ASSERT(cols > 1);

    // Move to the top of the grid.
    for (int row = 0; row < rows; row++)
    {
        append_local(
            path, path_size, path_size_capacity, x, y, row, 0, transposed);
    }

    // Raster horizontally until the adjacent to the origin.
    for (int row = rows - 1; row >= 1; row--)
    {
        if (((rows - 1 - row) & 1) == 0)
        {
            for (int col = 1; col < cols; col++)
            {
                append_local(path,
                             path_size,
                             path_size_capacity,
                             x,
                             y,
                             row,
                             col,
                             transposed);
            }
        }
        else
        {
            for (int col = cols - 1; col >= 1; col--)
            {
                append_local(path,
                             path_size,
                             path_size_capacity,
                             x,
                             y,
                             row,
                             col,
                             transposed);
            }
        }
    }

    for (int col = cols - 1; col >= 1; col--)
    {
        append_local(
            path, path_size, path_size_capacity, x, y, 0, col, transposed);
    }

    return;
}

/**
 * @brief Generate a horizontal modified raster for an odd grid.
 *
 * @p rows and @p cols must both be odd.
 */
static void
append_odd_unrotated_path (path_position_t *path,
                           size_t          *path_size,
                           size_t           path_size_capacity,
                           axis_t const    *x,
                           axis_t const    *y,
                           int              rows,
                           int              cols,
                           bool             transposed)
{
    int col;

    ASSERT(path != NULL);
    ASSERT(path_size != NULL);
    ASSERT(x != NULL);
    ASSERT(y != NULL);

    // Ensure an odd, two-dimensional grid.
    ASSERT((rows & 1) != 0);
    ASSERT((cols & 1) != 0);
    ASSERT(rows > 1);
    ASSERT(cols > 1);

    // Move to the top of the grid.
    for (int row = 0; row < rows; row++)
    {
        append_local(
            path, path_size, path_size_capacity, x, y, row, 0, transposed);
    }

    // Raster horizontally until at the right with two rows left.
    for (col = 1; col < cols; col++)
    {
        append_local(path,
                     path_size,
                     path_size_capacity,
                     x,
                     y,
                     rows - 1,
                     col,
                     transposed);
    }

    for (int row = rows - 2; row > 1; row--)
    {
        if (((rows - 2 - row) & 1) == 0)
        {
            for (col = cols - 1; col >= 1; col--)
            {
                append_local(path,
                             path_size,
                             path_size_capacity,
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
                             path_size,
                             path_size_capacity,
                             x,
                             y,
                             row,
                             col,
                             transposed);
            }
        }
    }

    // Move diagonally at the bottom-right corner to ensure a cyclic path.
    append_local(
        path, path_size, path_size_capacity, x, y, 1, cols - 1, transposed);

    append_local(
        path, path_size, path_size_capacity, x, y, 1, cols - 2, transposed);

    append_local(
        path, path_size, path_size_capacity, x, y, 0, cols - 1, transposed);

    col = cols - 2;

    // Squiggle toward the left until adjacent to the origin.
    while (col >= 2)
    {
        append_local(
            path, path_size, path_size_capacity, x, y, 0, col, transposed);

        append_local(
            path, path_size, path_size_capacity, x, y, 0, col - 1, transposed);

        append_local(
            path, path_size, path_size_capacity, x, y, 1, col - 1, transposed);

        append_local(
            path, path_size, path_size_capacity, x, y, 1, col - 2, transposed);

        col -= 2;
    }

    append_local(path, path_size, path_size_capacity, x, y, 0, 1, transposed);

    return;
}

/** @brief Determine whether a path changes direction at a point. */
static bool
corner_at (path_position_t const *prev,
           path_position_t const *curr,
           path_position_t const *next)
{
    ASSERT(prev != NULL);
    ASSERT(curr != NULL);
    ASSERT(next != NULL);

    int64_t dx_previous = (int64_t)curr->x - prev->x;
    int64_t dy_previous = (int64_t)curr->y - prev->y;
    int64_t dx_next     = (int64_t)next->x - curr->x;
    int64_t dy_next     = (int64_t)next->y - curr->y;

    int64_t cross_product = (dx_previous * dy_next) - (dy_previous * dx_next);
    int64_t dot_product   = (dx_previous * dx_next) + (dy_previous * dy_next);

    return cross_product != 0 || dot_product <= 0;
}

/** @brief Remove points that do not represent corners in a path. */
static void
shrink_to_corners (path_position_t *path, size_t *path_size)
{
    ASSERT(path != NULL);
    ASSERT(path_size != NULL);

    if (*path_size <= 2)
    {
        return;
    }

    size_t write_index = 1;

    for (size_t read_index = 1; read_index + 1 < *path_size; read_index++)
    {
        if (corner_at(&path[read_index - 1],
                      &path[read_index],
                      &path[read_index + 1]))
        {
            path[write_index] = path[read_index];
            write_index++;
        }
    }

    path[write_index] = path[*path_size - 1];
    write_index++;

    *path_size = write_index;

    return;
}
