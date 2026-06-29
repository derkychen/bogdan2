#ifndef PLATFORM_SAMD21G18A_ASSERT_H
#define PLATFORM_SAMD21G18A_ASSERT_H

#include <stdbool.h>

/** @brief Assertion structure that stores data. */
typedef struct
{
    /** The assertion expression that failed. */
    const char *expression;

    /** The file containing the failed assertion. */
    const char *file;

    /** The line containing the failed assertion. */
    int line;
} platform_samd21g18a_assert_info_t;

/** @brief Variable to inspect via debugger. */
extern volatile platform_samd21g18a_assert_info_t
    platform_samd21g18a_last_assert;

/** @brief Log assertion data and loop infinitely. */
void platform_samd21g18a_assert_fail(const char *expression,
                                     const char *file,
                                     int         line);

// Assert only for debugging builds.
#ifdef NDEBUG
#define PLATFORM_SAMD21G18A_ASSERT(expression) ((void)0)

#else
#define PLATFORM_SAMD21G18A_ASSERT(expression)                                \
    do                                                                        \
    {                                                                         \
        if (!(expression))                                                    \
        {                                                                     \
            platform_samd21g18a_assert_fail(#expression, __FILE__, __LINE__); \
        }                                                                     \
    } while (false)
#endif

#endif
