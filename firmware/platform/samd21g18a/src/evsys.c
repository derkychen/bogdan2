/**
 * @file evsys.c
 * @brief Implementation of EVSYS functionality.
 *
 * EVSYS works by routing a "channel" between users and generators. Users listen
 * to events that are created by generators.
 */
#include "platform/samd21g18a/evsys.h"
#include "platform/samd21g18a/assert.h"
#include "platform/samd21g18a/utils.h"
#include "sam.h" // IWYU pragma: keep
#include <stdbool.h>
#include <stdint.h>

static uint32_t const path_reg_values[EVSYS_PATH_COUNT] = {
    [EVSYS_PATH_SYNCHRONOUS]    = EVSYS_CHANNEL_PATH_SYNCHRONOUS,
    [EVSYS_PATH_RESYNCHRONIZED] = EVSYS_CHANNEL_PATH_RESYNCHRONIZED,
    [EVSYS_PATH_ASYNCHRONOUS]   = EVSYS_CHANNEL_PATH_ASYNCHRONOUS,
};

static uint16_t const channel_gclk_ids[EVSYS_CHANNEL_COUNT] = {
    [EVSYS_CHANNEL_0]  = GCLK_CLKCTRL_ID_EVSYS_0,
    [EVSYS_CHANNEL_1]  = GCLK_CLKCTRL_ID_EVSYS_1,
    [EVSYS_CHANNEL_2]  = GCLK_CLKCTRL_ID_EVSYS_2,
    [EVSYS_CHANNEL_3]  = GCLK_CLKCTRL_ID_EVSYS_3,
    [EVSYS_CHANNEL_4]  = GCLK_CLKCTRL_ID_EVSYS_4,
    [EVSYS_CHANNEL_5]  = GCLK_CLKCTRL_ID_EVSYS_5,
    [EVSYS_CHANNEL_6]  = GCLK_CLKCTRL_ID_EVSYS_6,
    [EVSYS_CHANNEL_7]  = GCLK_CLKCTRL_ID_EVSYS_7,
    [EVSYS_CHANNEL_8]  = GCLK_CLKCTRL_ID_EVSYS_8,
    [EVSYS_CHANNEL_9]  = GCLK_CLKCTRL_ID_EVSYS_9,
    [EVSYS_CHANNEL_10] = GCLK_CLKCTRL_ID_EVSYS_10,
    [EVSYS_CHANNEL_11] = GCLK_CLKCTRL_ID_EVSYS_11,
};

static uint16_t channels_in_use = 0u;

static inline bool     channel_valid(evsys_channel_t channel);
static inline bool     channel_not_used(evsys_channel_t channel);
static inline uint16_t channel_gclk_id(evsys_channel_t channel);
static inline bool     path_valid(evsys_path_t path);
static inline uint32_t path_reg_value(evsys_path_t path);

void
evsys_init (void)
{
    utils_apbc_enable(PM_APBCMASK_EVSYS);

    EVSYS->CTRL.reg = EVSYS_CTRL_SWRST;

    return;
}

evsys_channel_t
evsys_channel_claim (void)
{
    for (evsys_channel_t channel = 0; channel < EVSYS_CHANNEL_COUNT; channel++)
    {
        if ((channels_in_use & (1u << channel)) == 0u)
        {
            channels_in_use |= (uint16_t)(1u << channel);
            return channel;
        }
    }

    // No program should claim a channel when none are available.
    ASSERT(false);
}

void
evsys_channel_unclaim (evsys_channel_t channel)
{
    ASSERT(channel_valid(channel));

    channels_in_use &= ~(1u << channel);

    return;
}

void
evsys_channel_set_generator (evsys_channel_t   channel,
                             evsys_generator_t generator,
                             evsys_path_t      path)
{
    ASSERT(channel_valid(channel));
    ASSERT(channel_not_used(channel));
    ASSERT(path_valid(path));
    ASSERT(generator != 0u);

    if (path != EVSYS_PATH_ASYNCHRONOUS)
    {
        utils_gclk0_enable(channel_gclk_id(channel));
    }

    EVSYS->CHANNEL.reg = EVSYS_CHANNEL_CHANNEL((uint32_t)channel)
                         | EVSYS_CHANNEL_EVGEN((uint32_t)generator)
                         | path_reg_value(path);

    return;
}

void
evsys_channel_clear (evsys_channel_t channel)
{
    ASSERT(channel_valid(channel));

    EVSYS->CHANNEL.reg = EVSYS_CHANNEL_CHANNEL((uint32_t)channel);

    return;
}

void
evsys_user_set_channel (evsys_user_t user, evsys_channel_t channel)
{
    ASSERT(channel_valid(channel));

    EVSYS->USER.reg = EVSYS_USER_USER((uint32_t)user)
                      | EVSYS_USER_CHANNEL((uint32_t)channel + 1u);

    return;
}

void
evsys_user_clear (evsys_user_t user)
{
    EVSYS->USER.reg = EVSYS_USER_USER((uint32_t)user);

    return;
}

/** @brief Check whether a channel is valid. */
static inline bool
channel_valid (evsys_channel_t channel)
{
    return channel < EVSYS_CHANNEL_COUNT;
}

/** @brief Check whether a channel is in use. */
static inline bool
channel_not_used (evsys_channel_t channel)
{
    return channel
}

/** @brief Set a channel as used. */
static inline bool
channel_set_used (evsys_channel_t channel)
{
    return channel
}

/** @brief Get the GCLK ID for a channel. */
static inline uint16_t
channel_gclk_id (evsys_channel_t channel)
{
    ASSERT(channel_valid(channel));

    return channel_gclk_ids[channel];
}

/** @brief Check whether a path is valid. */
static inline bool
path_valid (evsys_path_t path)
{
    return path < EVSYS_PATH_COUNT;
}

/** @brief Convert a path to its register value. */
static inline uint32_t
path_reg_value (evsys_path_t path)
{
    ASSERT(path_valid(path));

    return path_reg_values[path];
}
