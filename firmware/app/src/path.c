/**
 * @file path.c
 * @brief Implementation of incremental modified-raster generation.
 */
#include "app/path.h"
#include "platform/samd21g18a/assert.h"
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>

/** @brief Internal structure for a modified raster local segment. */
typedef struct
{
    path_indices_t start; /**< Starting indices of the segment. */
    path_indices_t end;   /**< Ending indices of the segment. */
} segment_t;

static int                     num_points(int min, int max);
static int                     anchor_index(int min_coord, int max_coord);
static path_raster_direction_t select_raster_direction(
    path_raster_direction_t prev_raster_direction,
    int                     x_num_points,
    int                     y_num_points);
static int get_line_col(int current, int anchor, int last, bool corners_only);
static segment_t      create_segment(int start_row,
                                     int start_col,
                                     int end_row,
                                     int end_col);
static segment_t      get_even_segment(path_indices_t curr,
                                       int            num_rows,
                                       int            num_cols);
static segment_t      get_odd_segment(path_indices_t curr,
                                      int            num_rows,
                                      int            num_cols);
static bool           index_between(int index, int start, int end);
static bool           segment_contains_anchor(segment_t const *segment,
                                              path_indices_t   anchor);
static int            unit_step(int displacement);
static path_indices_t advance_line(bool corners_only,
                                   int  curr_col,
                                   int  anchor_col,
                                   int  num_cols);
static path_indices_t advance_raster(bool           corners_only,
                                     path_indices_t curr,
                                     path_indices_t anchor,
                                     int            num_rows,
                                     int            num_cols);
static path_indices_t advance(bool           corners_only,
                              path_indices_t curr,
                              path_indices_t anchor,
                              int            num_rows,
                              int            num_cols);
static path_coords_t  indices_to_coords(
    path_indices_t          indices,
    path_coords_t           zero,
    path_raster_direction_t raster_direction);

path_status_t
path_init (path_t                  *path,
           bool                     corners_only,
           path_raster_direction_t *prev_raster_direction,
           int                      x_min,
           int                      x_max,
           int                      y_min,
           int                      y_max)
{
    ASSERT(path != NULL);

    if ((x_min > x_max) || (y_min > y_max))
    {
        return PATH_STATUS_ERR_BOUNDS_MIN_GREATER_THAN_MAX;
    }

    if (x_min < PATH_COORD_MIN || x_max > PATH_COORD_MAX
        || y_min < PATH_COORD_MIN || y_max > PATH_COORD_MAX)
    {
        return PATH_STATUS_ERR_BOUNDS_TOO_LARGE;
    }

    if ((x_max - x_min) == INT_MAX || (y_max - y_min) == INT_MAX)
    {
        return PATH_STATUS_ERR_BOUNDS_TOO_LARGE;
    }

    path->corners_only = corners_only;

    int x_num_points = num_points(x_min, x_max);
    int y_num_points = num_points(y_min, y_max);

    path->raster_direction = select_raster_direction(
        *prev_raster_direction, x_num_points, y_num_points);

    path->phase = PATH_PHASE_READY;

    path->zero = (path_coords_t) { .x = x_min, .y = y_min };

    int anchor_x_index = anchor_index(x_min, x_max);
    int anchor_y_index = anchor_index(y_min, y_max);

    if (path->raster_direction == PATH_RASTER_DIRECTION_HORIZONTAL)
    {
        path->anchor
            = (path_indices_t) { .row = anchor_y_index, .col = anchor_x_index };
        path->num_rows = y_num_points;
        path->num_cols = x_num_points;
    }
    else
    {
        path->anchor
            = (path_indices_t) { .row = anchor_x_index, .col = anchor_y_index };
        path->num_rows = x_num_points;
        path->num_cols = y_num_points;
    }

    ASSERT(path->num_rows >= 1);
    ASSERT(path->num_cols >= 1);
    ASSERT(path->anchor.row >= 0 && path->anchor.row < path->num_rows);
    ASSERT(path->anchor.col >= 0 && path->anchor.col < path->num_cols);

    if (path->num_rows > 1)
    {
        ASSERT(path->num_cols > 1);
        ASSERT(((path->num_rows & 1) == 0) || ((path->num_cols & 1) != 0));
    }

    path->curr = path->anchor;

    // Only two-dimensional grids cause raster direction alternation.
    if (x_num_points > 1 && y_num_points > 1)
    {
        *prev_raster_direction = path->raster_direction;
    }

    return PATH_STATUS_OK;
}

path_status_t
path_next (path_t *path, path_coords_t *coords)
{
    ASSERT(path != NULL);
    ASSERT(coords != NULL);

    ASSERT(path->phase == PATH_PHASE_READY || path->phase == PATH_PHASE_ONGOING
           || path->phase == PATH_PHASE_DONE);

    if (path->phase == PATH_PHASE_DONE)
    {
        return PATH_STATUS_DONE;
    }

    ASSERT(path->raster_direction == PATH_RASTER_DIRECTION_HORIZONTAL
           || path->raster_direction == PATH_RASTER_DIRECTION_VERTICAL);
    ASSERT(path->num_rows >= 1);
    ASSERT(path->num_cols >= 1);
    ASSERT(path->anchor.row >= 0 && path->anchor.row < path->num_rows);
    ASSERT(path->anchor.col >= 0 && path->anchor.col < path->num_cols);
    ASSERT(path->curr.row >= 0 && path->curr.row < path->num_rows);
    ASSERT(path->curr.col >= 0 && path->curr.col < path->num_cols);

    // Manage traversal phase.
    //
    // Phase evolves as `READY` (before generation) to `ONGOING` (during
    // generation) to `DONE` (generation completed).
    if (path->phase == PATH_PHASE_READY)
    {
        if (path->num_rows == 1 && path->num_cols == 1)
        {
            path->phase = PATH_PHASE_DONE;
        }
        else
        {
            path->phase = PATH_PHASE_ONGOING;
        }
    }
    else
    {
        ASSERT(path->phase == PATH_PHASE_ONGOING);

        path->curr = advance(path->corners_only,
                             path->curr,
                             path->anchor,
                             path->num_rows,
                             path->num_cols);

        if (path->curr.row == path->anchor.row
            && path->curr.col == path->anchor.col)
        {
            path->phase = PATH_PHASE_DONE;
        }
    }

    // Convert the local indices into the actual coordinate.
    *coords = indices_to_coords(path->curr, path->zero, path->raster_direction);

    return PATH_STATUS_OK;
}

/** @brief Get the number of points on a side of the grid. */
static int
num_points (int min, int max)
{
    ASSERT(min >= PATH_COORD_MIN);
    ASSERT(max <= PATH_COORD_MAX);
    ASSERT(min <= max);
    ASSERT((max - min) < INT_MAX);

    return (max - min) + 1;
}

/**
 * @brief Get the local index of the path anchor.
 *
 * This function returns the local index of the coordinate zero, or the
 * coordinate nearest to zero given a set of bound coordinates. That is, this
 * function computes the index of one of the coordinates of the anchor.
 */
static int
anchor_index (int min_coord, int max_coord)
{
    ASSERT(min_coord >= PATH_COORD_MIN);
    ASSERT(max_coord <= PATH_COORD_MAX);
    ASSERT(min_coord <= max_coord);

    if (min_coord > 0)
    {
        return 0;
    }

    if (max_coord < 0)
    {
        return max_coord - min_coord;
    }

    return -min_coord;
}

/**
 * @brief Choose the raster direction.
 *
 * This selection is based in the user supplied previous raster direction as
 * well as path geometry.
 */
static path_raster_direction_t
select_raster_direction (path_raster_direction_t prev_raster_direction,
                         int                     x_num_points,
                         int                     y_num_points)
{
    ASSERT(x_num_points >= 1);
    ASSERT(y_num_points >= 1);

    // For grids that have only one position, the raster direction is
    // irrelevant, so the previous one is used.
    if (x_num_points == 1 && y_num_points == 1)
    {
        return prev_raster_direction;
    }

    // For one-dimensional grids, the raster direction is parallel to the line.
    if (y_num_points == 1)
    {
        return PATH_RASTER_DIRECTION_HORIZONTAL;
    }

    if (x_num_points == 1)
    {
        return PATH_RASTER_DIRECTION_VERTICAL;
    }

    // For two-dimensional grids, if the dimensions of the grid are both even or
    // both odd, either raster direction can be chosen, so the opposite of the
    // previous one is used. Otherwise, the raster direction must be parallel to
    // the odd side.
    if ((x_num_points & 1) == (y_num_points & 1))
    {
        return (prev_raster_direction == PATH_RASTER_DIRECTION_HORIZONTAL)
                   ? PATH_RASTER_DIRECTION_VERTICAL
                   : PATH_RASTER_DIRECTION_HORIZONTAL;
    }

    return ((x_num_points & 1) == 0) ? PATH_RASTER_DIRECTION_VERTICAL
                                     : PATH_RASTER_DIRECTION_HORIZONTAL;
}

/** @brief Handle a one-dimensional grid. */
static int
get_line_col (int current, int anchor, int last, bool corners_only)
{
    ASSERT(last >= 1);
    ASSERT(anchor >= 0 && anchor <= last);
    ASSERT(current >= 0 && current <= last);

    int near = (anchor <= last - anchor) ? 0 : last;
    int far  = last - near;

    if (corners_only)
    {
        if (current == anchor)
        {
            return (anchor == 0 || anchor == last) ? far : near;
        }

        if (current == near)
        {
            return far;
        }

        ASSERT(current == far);

        return anchor;
    }

    if (near == 0)
    {
        if (current >= 1 && current <= anchor)
        {
            return current - 1;
        }

        if (current == 0)
        {
            return anchor + 1;
        }

        if (current < last)
        {
            return current + 1;
        }

        ASSERT(current == last);

        return anchor;
    }

    if (current >= anchor && current < last)
    {
        return current + 1;
    }

    if (current == last)
    {
        return anchor - 1;
    }

    if (current >= 1 && current < anchor)
    {
        return current - 1;
    }

    ASSERT(current == 0);

    return anchor;
}

/** @brief Create a local segment with start and end rows and columns. */
static segment_t
create_segment (int start_row, int start_col, int end_row, int end_col)
{
    ASSERT(start_row >= 0);
    ASSERT(start_col >= 0);
    ASSERT(end_row >= 0);
    ASSERT(end_col >= 0);
    ASSERT(start_row != end_row || start_col != end_col);

    return (segment_t) {
        .start = {
            .row = start_row,
            .col = start_col,
        },
        .end = {
            .row = end_row,
            .col = end_col,
        },
    };
}

/**
 * @brief Get a segment on an even two-dimensional raster.
 *
 * The even raster looks like a block letter E with an arbitrary number of
 * horizontal "prongs" in the local space.
 */
static segment_t
get_even_segment (path_indices_t curr, int num_rows, int num_cols)
{
    ASSERT(num_rows > 1);
    ASSERT((num_rows & 1) == 0);
    ASSERT(num_cols > 1);
    ASSERT(curr.row >= 0 && curr.row < num_rows);
    ASSERT(curr.col >= 0 && curr.col < num_cols);

    int last_row = num_rows - 1;
    int last_col = num_cols - 1;

    // NOTE: A two-column even raster collapses to a rectangle. This must be
    //      handled in a separate case since the row turns become one segment.
    if (num_cols == 2)
    {
        // Handle the local left side of the rectangle.
        if (curr.col == 0 && curr.row < last_row)
        {
            return create_segment(curr.row, curr.col, last_row, curr.col);
        }

        // Handle the local top side of the rectangle.
        if (curr.col == 0)
        {
            ASSERT(curr.row == last_row);

            return create_segment(curr.row, curr.col, curr.row, 1);
        }

        // Handle the local right side of the rectangle.
        if (curr.row > 0)
        {
            return create_segment(curr.row, curr.col, 0, curr.col);
        }

        // Handle the local bottom side of the rectangle.
        return create_segment(curr.row, curr.col, curr.row, 0);
    }

    // Handle the local left side of the raster (| part of the E).
    if (curr.col == 0)
    {
        return (curr.row < last_row)
                   ? create_segment(curr.row, curr.col, last_row, curr.col)
                   : create_segment(curr.row, curr.col, curr.row, last_col);
    }

    // Handle the local bottom of the raster (_ part of the E).
    if (curr.row == 0)
    {
        return create_segment(curr.row, curr.col, curr.row, 0);
    }

    // Handle the prongs in the raster (-s in the E).
    int end_col = ((curr.row & 1) != 0) ? last_col : 1;

    return (curr.col != end_col)
               ? create_segment(curr.row, curr.col, curr.row, end_col)
               : create_segment(curr.row, curr.col, curr.row - 1, curr.col);
}

/**
 * @brief Get a segment on an odd two-dimensional raster.
 *
 * The odd raster looks like a block letter E with an arbitrary number of
 * horizontal "prongs" in the local space. It differs from the even raster at
 * the local bottom, where the bottom side of the bottom prong contains one
 * diagonal and a "squiggle" back to the local origin.
 */
static segment_t
get_odd_segment (path_indices_t curr, int num_rows, int num_cols)
{
    ASSERT(num_rows > 1);
    ASSERT((num_rows & 1) != 0);
    ASSERT(num_cols > 1);
    ASSERT((num_cols & 1) != 0);
    ASSERT(curr.row >= 0 && curr.row < num_rows);
    ASSERT(curr.col >= 0 && curr.col < num_cols);

    int last_row = num_rows - 1;
    int last_col = num_cols - 1;

    // Handle the local left side of the raster (| part of the E).
    if (curr.col == 0)
    {
        return (curr.row < last_row)
                   ? create_segment(curr.row, curr.col, last_row, curr.col)
                   : create_segment(curr.row, curr.col, curr.row, last_col);
    }

    // Handle the prongs in the raster (-s in the E).
    if (curr.row >= 2)
    {
        int end_col = ((curr.row & 1) != 0) ? 1 : last_col;

        return (curr.col != end_col)
                   ? create_segment(curr.row, curr.col, curr.row, end_col)
                   : create_segment(curr.row, curr.col, curr.row - 1, curr.col);
    }

    // Handle the local downward, upper horizontal, and diagonal segments at
    // bottom prong squiggle (bottom of the _ in the E).
    if (curr.row == 1)
    {
        if ((curr.col & 1) == 0)
        {
            return create_segment(curr.row, curr.col, curr.row, curr.col - 1);
        }

        return (curr.col == last_col - 1)
                   ? create_segment(curr.row, curr.col, 0, last_col)
                   : create_segment(curr.row, curr.col, 0, curr.col);
    }

    // Handle the last segment that returns to the local origin.
    ASSERT(curr.row == 0);

    if (curr.col == 1)
    {
        return create_segment(curr.row, curr.col, curr.row, 0);
    }

    // Handle the segment two units in length that is formed after the diagonal.
    if (curr.col == last_col)
    {
        return create_segment(curr.row, curr.col, curr.row, curr.col - 2);
    }

    // Handle the local upward and lower horizontal segments at the local bottom
    // side of the bottom prong squiggle (bottom of the _ in the E).
    return ((curr.col & 1) == 0)
               ? create_segment(curr.row, curr.col, 1, curr.col)
               : create_segment(curr.row, curr.col, curr.row, curr.col - 1);
}

/** @brief Check if an index lies between a start and an end index. */
static bool
index_between (int index, int start, int end)
{
    ASSERT(start != end);

    return (start < end) ? (start < index && index <= end)
                         : (end <= index && index < start);
}

/** @brief Check if a segment contains the anchor. */
static bool
segment_contains_anchor (segment_t const *segment, path_indices_t anchor)
{
    ASSERT(segment != NULL);

    if (segment->start.row == segment->end.row)
    {
        return anchor.row == segment->start.row
               && index_between(
                   anchor.col, segment->start.col, segment->end.col);
    }

    if (segment->start.col == segment->end.col)
    {
        return anchor.col == segment->start.col
               && index_between(
                   anchor.row, segment->start.row, segment->end.row);
    }

    // The only remaining case should be the diagonal for odd grids, whose
    // endpoints are the only place the anchor can lie.
    ASSERT(segment->end.row == segment->start.row - 1);
    ASSERT(segment->end.col == segment->start.col + 1);

    return anchor.row == segment->end.row && anchor.col == segment->end.col;
}

/** @brief Convert a displacement into a unit step with the same sign. */
static int
unit_step (int displacement)
{
    return (displacement > 0) - (displacement < 0);
}

/** @brief Load the next position to visit for a one-dimensional grid. */
static path_indices_t
advance_line (bool corners_only, int curr_col, int anchor_col, int num_cols)
{
    ASSERT(num_cols > 1);

    return (path_indices_t) {
        .row = 0,
        .col = get_line_col(curr_col, anchor_col, num_cols - 1, corners_only),
    };
}

/** @brief Load the next position to visit for a two-dimensional grid. */
static path_indices_t
advance_raster (bool           corners_only,
                path_indices_t curr,
                path_indices_t anchor,
                int            num_rows,
                int            num_cols)
{
    ASSERT(num_rows > 1);
    ASSERT(num_cols > 1);
    ASSERT(anchor.row >= 0 && anchor.row < num_rows);
    ASSERT(anchor.col >= 0 && anchor.col < num_cols);
    ASSERT(curr.row >= 0 && curr.row < num_rows);
    ASSERT(curr.col >= 0 && curr.col < num_cols);

    segment_t segment = ((num_rows & 1) == 0)
                            ? get_even_segment(curr, num_rows, num_cols)
                            : get_odd_segment(curr, num_rows, num_cols);

    ASSERT(segment.start.row == curr.row);
    ASSERT(segment.start.col == curr.col);

    // Stop the closing segment at the anchor.
    if (segment_contains_anchor(&segment, anchor))
    {
        segment.end = anchor;
    }

    // Use unit steps for a full raster.
    if (!corners_only)
    {
        segment.end.row = segment.start.row
                          + unit_step(segment.end.row - segment.start.row);
        segment.end.col = segment.start.col
                          + unit_step(segment.end.col - segment.start.col);
    }

    ASSERT(segment.end.row >= 0 && segment.end.row < num_rows);
    ASSERT(segment.end.col >= 0 && segment.end.col < num_cols);

    return segment.end;
}

/** @brief Load the next position to visit. */
static path_indices_t
advance (bool           corners_only,
         path_indices_t curr,
         path_indices_t anchor,
         int            num_rows,
         int            num_cols)
{
    ASSERT(num_rows >= 1);
    ASSERT(num_cols > 1);
    ASSERT(anchor.row >= 0 && anchor.row < num_rows);
    ASSERT(anchor.col >= 0 && anchor.col < num_cols);
    ASSERT(curr.row >= 0 && curr.row < num_rows);
    ASSERT(curr.col >= 0 && curr.col < num_cols);

    if (num_rows == 1)
    {
        return advance_line(corners_only, curr.col, anchor.col, num_cols);
    }

    return advance_raster(corners_only, curr, anchor, num_rows, num_cols);
}

/** @brief Convert local indices to coordinates. */
static path_coords_t
indices_to_coords (path_indices_t          indices,
                   path_coords_t           zero,
                   path_raster_direction_t raster_direction)
{
    ASSERT(indices.row >= 0);
    ASSERT(indices.col >= 0);
    ASSERT(raster_direction == PATH_RASTER_DIRECTION_HORIZONTAL
           || raster_direction == PATH_RASTER_DIRECTION_VERTICAL);

    if (raster_direction == PATH_RASTER_DIRECTION_HORIZONTAL)
    {
        return (path_coords_t) {
            .x = zero.x + indices.col,
            .y = zero.y + indices.row,
        };
    }

    return (path_coords_t) {
        .x = zero.x + indices.row,
        .y = zero.y + indices.col,
    };
}
