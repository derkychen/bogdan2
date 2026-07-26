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
} platform_samd21g18a_assert_data_t;

/** @brief Variable to inspect via debugger. */
extern platform_samd21g18a_assert_data_t volatile platform_samd21g18a_assert_data;

/**
 * @brief Record assertion data and loop infinitely.
 *
 * NOTE: Only the following macro that wraps this function should be
 *       invoked.This function should never be called directly.
 */
void platform_samd21g18a_assert_fail(char const *expression,
                                     char const *file,
                                     int         line);

// Assert only for debugging builds.
#ifdef NDEBUG
#define PLATFORM_SAMD21G18A_ASSERT(condition) ((void)sizeof(condition))
#else
#define PLATFORM_SAMD21G18A_ASSERT(condition)                                \
    do                                                                       \
    {                                                                        \
        if (!(condition))                                                    \
        {                                                                    \
            platform_samd21g18a_assert_fail(#condition, __FILE__, __LINE__); \
        }                                                                    \
    } while (0)
#endif

#endif
