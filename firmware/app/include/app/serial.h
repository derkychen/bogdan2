#ifndef APP_SERIAL_H
#define APP_SERIAL_H

#include <stdbool.h>
#include <stddef.h>

#define APP_SERIAL_READ_BUFFER_SIZE (512u)

/** @brief Serial status codes. */
typedef enum
{
    APP_SERIAL_STATUS_OK = 0,
    APP_SERIAL_STATUS_OK_LINE_RECEIVED,
    APP_SERIAL_STATUS_OK_LINE_PENDING,
    APP_SERIAL_STATUS_ERR,
    APP_SERIAL_STATUS_ERR_DISCONNECTED,
    APP_SERIAL_STATUS_ERR_LINE_BUFFER_OVERFLOW,
    APP_SERIAL_STATUS_ERR_LINE_WRITE_FAILED,
} app_serial_status_t;

/** @brief Initialize app-side serial processing, reset the line buffer. */
void app_serial_init(void);

/**
 * @brief Read one newline-terminated line from the RX buffer.
 *
 * This function is non-blocking. It accumulates characters internally and
 * returns only when a full line has been received. The returned line does not
 * contain the trailing newline character. It returns the "received" status code
 * when a full line was copied into @p buffer.
 */
app_serial_status_t app_serial_read_line(char *buffer, size_t buffer_size);

/** @brief Write one newline-terminated line to the TX buffer. */
app_serial_status_t app_serial_write_line(char const *message);

#endif
