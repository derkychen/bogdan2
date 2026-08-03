#ifndef PLATFORM_SAMD21G18A_ASSERT_H
#define PLATFORM_SAMD21G18A_ASSERT_H

/** @brief Assertion structure that stores data. */
typedef struct
{
    /** The assertion expression that failed. */
    char const *expression;

    /** The file containing the failed assertion. */
    char const *file;

    /** The line containing the failed assertion. */
    int line;
} assert_data_t;

/** @brief Variable to inspect via debugger. */
extern assert_data_t volatile assert_data;

/**
 * @brief Record assertion data and loop infinitely.
 *
 * NOTE: Only the following macro that wraps this function should be
 *       invoked.This function should never be called directly.
 */
void assert_fail(char const *expression, char const *file, int line);

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
