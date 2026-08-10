/**
 * @file assert.h
 * @brief Asserting functionality.
 *
 * This module provides an assertion macro that traps the processor in a
 * breakpoint if an assertion fails. It records assertion data into a global
 * variable. This can be accessed if a debugging chip is used.
 *
 * WARNING: Changes to this file should be made with caution, as it contains
 *          low-level logic that can be broken.
 *
 * NOTE: This macro should only be used for checking conditions that fail if
 *       there is a programming error. It should not be used for handling errors
 *       that are expected.
 */
#ifndef PLATFORM_SAMD21G18A_ASSERT_H
#define PLATFORM_SAMD21G18A_ASSERT_H

/** @brief Assertion structure that stores data. */
typedef struct
{
    char const *expression; /**< The assertion expression that failed. */
    char const *file;       /**< The file containing the failed assertion. */
    int         line;       /**< The line containing the failed assertion. */
} assert_data_t;

/** @brief Variable to inspect via debugger. */
extern assert_data_t volatile assert_data;

/**
 * @brief Record assertion data and loop infinitely.
 *
 * NOTE: Only the following macro that wraps this function should be
 *       invoked.This function should never be called directly.
 */
_Noreturn void assert_fail(char const *expression, char const *file, int line);

// Assert only for debugging builds.
#ifdef NDEBUG
#define ASSERT(condition) ((void)sizeof(condition))
#else
#define ASSERT(condition)                                \
    do                                                   \
    {                                                    \
        if (!(condition))                                \
        {                                                \
            assert_fail(#condition, __FILE__, __LINE__); \
        }                                                \
    } while (0)
#endif

#endif
