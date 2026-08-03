/**
 * @file eic.h
 * @brief SAMD21G18A EIC-related functionality.
 *
 * This module initializes the EIC peripheral and provides functions to
 * configure, enable, and disable interrupts as well as the EIC's event
 * generation.
 */
#ifndef PLATFORM_SAMD21G18A_EIC_H
#define PLATFORM_SAMD21G18A_EIC_H

#include "platform/samd21g18a/evsys.h"
#include "platform/samd21g18a/pin.h"
#include <stdbool.h>
#include <stdint.h>

/** @brief Type for external interrupt line. */
typedef uint8_t eic_extint_line_t;

/** @brief Enumeration for interrupt sensing. */
typedef enum
{
    EIC_SENSE_NONE = 0,
    EIC_SENSE_RISE,
    EIC_SENSE_FALL,
    EIC_SENSE_BOTH,
    EIC_SENSE_HIGH,
    EIC_SENSE_LOW,

    EIC_SENSE_COUNT,
} eic_sense_t;

/**
 * @brief EIC callback format.
 *
 * The context pointer is used to pass any relevant information.
 */
typedef void (*eic_callback_t)(eic_extint_line_t line, void *context);

/** @brief EIC pin data. */
typedef struct
{
    /** Pin. */
    pin_t const *pin;

    /**
     * External interrupt line.
     *
     * NOTE: This is designated for each pin, it is not arbitrary.
     */
    eic_extint_line_t line;
} eic_pin_t;

/** @brief Check if an external interrupt line is valid. */
bool eic_extint_line_valid(eic_extint_line_t line);

/** @brief Check if a sense value is valid. */
bool eic_sense_valid(eic_sense_t sense);

/** @brief Poll the EIC until it is ready. */
void eic_poll_sync(void);

/** @brief Initialize the EIC peripheral. */
void eic_init(void);

/** @brief Configure a pin connected to the EIC. */
void eic_configure(eic_pin_t const *eic_pin, eic_sense_t sense);

/** @brief Register a callback for a pin that runs on every interrupt. */
void eic_register_callback_entry(eic_extint_line_t line,
                                 eic_callback_t    callback,
                                 void             *context);

/** @brief Disable interrupts on a external interrupt line. */
void eic_interrupt_disable(eic_extint_line_t line);

/** @brief Enable interrupts on a external interrupt line. */
void eic_interrupt_enable(eic_extint_line_t line);

/** @brief Disable event generator for an external interrupt line. */
void eic_event_disable(eic_extint_line_t line);

/** @brief Enable event generator for an external interrupt line. */
void eic_event_enable(eic_extint_line_t line);

/** @brief Get the event generator for an external interrupt line. */
evsys_generator_t eic_event_generator(eic_extint_line_t line);

#endif
