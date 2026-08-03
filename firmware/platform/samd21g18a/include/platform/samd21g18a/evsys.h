/**
 * @file evsys.h
 * @brief EVSYS peripheral functionality.
 *
 * This module provides functions to configure event routing through the
 * SAMD21G18A Event System (EVSYS).
 */
#ifndef PLATFORM_SAMD21G18A_EVSYS_H
#define PLATFORM_SAMD21G18A_EVSYS_H

#include <stdint.h>

/** @brief EVSYS event path. */
typedef enum
{
    EVSYS_PATH_SYNCHRONOUS = 0,
    EVSYS_PATH_RESYNCHRONIZED,
    EVSYS_PATH_ASYNCHRONOUS,

    EVSYS_PATH_COUNT,
} evsys_path_t;

/** @brief Enumerate EVSYS channels. */
typedef enum
{
    EVSYS_CHANNEL_0 = 0,
    EVSYS_CHANNEL_1,
    EVSYS_CHANNEL_2,
    EVSYS_CHANNEL_3,
    EVSYS_CHANNEL_4,
    EVSYS_CHANNEL_5,
    EVSYS_CHANNEL_6,
    EVSYS_CHANNEL_7,
    EVSYS_CHANNEL_8,
    EVSYS_CHANNEL_9,
    EVSYS_CHANNEL_10,
    EVSYS_CHANNEL_11,

    EVSYS_CHANNEL_COUNT,
} evsys_channel_t;

/** @brief Event generator identifier. */
typedef uint8_t evsys_generator_t;

/** @brief Event user identifier. */
typedef uint8_t evsys_user_t;

/** @brief Initialize the EVSYS peripheral. */
void evsys_init(void);

/** @brief Set a channel's event generator and path. */
void evsys_channel_set_generator(evsys_channel_t   channel,
                                 evsys_generator_t generator,
                                 evsys_path_t      path);

/** @brief Disconnect a channel from an event generator. */
void evsys_channel_clear(evsys_channel_t channel);

/** @brief Set a user's channel. */
void evsys_user_set_channel(evsys_user_t user, evsys_channel_t channel);

/** @brief Disconnect a user from a channel. */
void evsys_user_clear(evsys_user_t user);

#endif
