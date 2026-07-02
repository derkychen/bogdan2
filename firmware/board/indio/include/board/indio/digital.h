#ifndef BOARD_INDIO_DIGITAL_H
#define BOARD_INDIO_DIGITAL_H

#include <stdint.h>

#define BOARD_INDIO_DIGITAL_DEFAULT_CFG      (0xFFFFU)
#define BOARD_INDIO_DIGITAL_DEFAULT_OUTPUTS  (0x0000U)
#define BOARD_INDIO_DIGITAL_DEFAULT_POLARITY (0x0000U)

/** @brief Enumeration of IND.I/O pins. */
typedef enum
{
    BOARD_INDIO_DIGITAL_PIN_CH1 = 0,
    BOARD_INDIO_DIGITAL_PIN_CH2,
    BOARD_INDIO_DIGITAL_PIN_CH3,
    BOARD_INDIO_DIGITAL_PIN_CH4,
    BOARD_INDIO_DIGITAL_PIN_CH5,
    BOARD_INDIO_DIGITAL_PIN_CH6,
    BOARD_INDIO_DIGITAL_PIN_CH7,
    BOARD_INDIO_DIGITAL_PIN_CH8,

    BOARD_INDIO_DIGITAL_PIN_COUNT,
} board_indio_digital_pin_t;

/** @brief Enumeration of pin configurations. */
typedef enum
{
    BOARD_INDIO_DIGITAL_CFG_OUTPUT = 0,
    BOARD_INDIO_DIGITAL_CFG_INPUT  = 1,
} board_indio_digital_pin_cfg_t;

/** @brief Enumeration of pin polarities. */
typedef enum
{
    BOARD_INDIO_DIGITAL_POLARITY_NORMAL   = 0,
    BOARD_INDIO_DIGITAL_POLARITY_INVERTED = 1,
} board_indio_digital_pin_polarity_t;

/** @brief Enumeration of pin levels. */
typedef enum
{
    BOARD_INDIO_DIGITAL_LEVEL_LOW  = 0,
    BOARD_INDIO_DIGITAL_LEVEL_HIGH = 1,
} board_indio_digital_pin_level_t;

/** @brief Digital status codes. */
typedef enum
{
    BOARD_INDIO_DIGITAL_STATUS_OK = 0,
    BOARD_INDIO_DIGITAL_STATUS_ERR,
} board_indio_digital_status_t;

/** @brief Digital pin mask. */
typedef uint16_t board_indio_digital_mask_t;

#define BOARD_INDIO_DIGITAL_INTERRUPT_MASK_FROM_PIN(pin) \
    ((board_indio_digital_mask_t)(1U << (pin)))

/** @brief Digital pin interrupt callback format. */
typedef void (*board_indio_digital_mask_interrupt_callback_t)(
    board_indio_digital_mask_t mask, void *context);

/** @brief Digital interrupt configuration for a pin. */
typedef struct
{
    /** Pins whose inputs generate interrupts. */
    board_indio_digital_mask_t allowed_mask;

    /** Pins whose interrupt source should latch. */
    board_indio_digital_mask_t latched_mask;
} board_indio_digital_interrupt_cfg_t;

/** @brief Initialize the digital pins. */
board_indio_digital_status_t board_indio_digital_init(void);

/**
 * @brief Manage the digital pins.
 *
 * NOTE: This function needs to be called from the main loop periodically.
 */
board_indio_digital_status_t board_indio_digital_task(void);

/**
 * @brief Register a callback that runs when a digital pin receives an
 *        interrupt.
 */
void board_indio_digital_interrupt_pin_register_callback(
    board_indio_digital_mask_t                    mask,
    board_indio_digital_mask_interrupt_callback_t callback,
    void                                         *context);

/** @brief Configure behaviour with pin masks. */
board_indio_digital_status_t board_indio_digital_interrupt_configure(
    board_indio_digital_interrupt_cfg_t const *cfg);

/** @brief Read latched interrupt source bits. */
board_indio_digital_status_t board_indio_digital_interrupt_source_read(
    board_indio_digital_mask_t *mask);

/** @brief Clear/re-arm pending digital interrupt source state. */
board_indio_digital_status_t board_indio_digital_interrupt_source_drain(
    board_indio_digital_mask_t mask);

/** @brief Set the configuration (output or input) of a pin. */
board_indio_digital_status_t board_indio_digital_pin_set_cfg(
    board_indio_digital_pin_t pin, board_indio_digital_pin_cfg_t cfg);

/** @brief Set the polarity (normal or inverted) of a pin. */
board_indio_digital_status_t board_indio_digital_pin_set_polarity(
    board_indio_digital_pin_t pin, board_indio_digital_pin_polarity_t polarity);

/** @brief Write the value (LOW or HIGH) of a pin. */
board_indio_digital_status_t board_indio_digital_pin_write(
    board_indio_digital_pin_t pin, board_indio_digital_pin_level_t level);

/** @brief Read the value (LOW or HIGH) from a pin. */
board_indio_digital_status_t board_indio_digital_pin_read(
    board_indio_digital_pin_t pin, board_indio_digital_pin_level_t *level);

#endif
