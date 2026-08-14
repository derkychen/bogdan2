/**
 * @file serial.h
 * @brief Serial functionality.
 *
 * This module provides serial line reading and writing functionality, allowing
 * communication with the host.
 */
#ifndef APP_SERIAL_H
#define APP_SERIAL_H

#include <stdbool.h>
#include <stddef.h>

#define SERIAL_READ_BUFFER_SIZE (512u)

/** @brief Serial status codes. */
typedef enum
{
    SERIAL_STATUS_OK = 0,
    SERIAL_STATUS_OK_LINE_RECEIVED,
    SERIAL_STATUS_OK_LINE_PENDING,
    SERIAL_STATUS_ERR,
    SERIAL_STATUS_ERR_POLL_TIMEOUT,
    SERIAL_STATUS_ERR_DISCONNECTED,
    SERIAL_STATUS_ERR_LINE_BUFFER_OVERFLOW,
    SERIAL_STATUS_ERR_LINE_WRITE_FAILED,
} serial_status_t;

/** @brief Initialize app-side serial processing, reset the line buffer. */
void serial_init(void);

/**
 * @brief Read one newline-terminated line from the RX buffer.
 *
 * This function is non-blocking. It accumulates characters internally and
 * returns only when a full line has been received. The returned line does not
 * contain the trailing newline character. It returns the "received" status code
 * when a full line was copied into @p buffer.
 */
serial_status_t serial_read_line(char *buffer, size_t buffer_size);

/** @brief Write one newline-terminated line to the TX buffer. */
serial_status_t serial_write_line(char const *message);

#endif
