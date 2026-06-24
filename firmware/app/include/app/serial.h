#ifndef APP_SERIAL_H
#define APP_SERIAL_H

#include <stdbool.h>
#include <stddef.h>

#define APP_SERIAL_READ_BUFFER_SIZE (512u)

/** @brief Initialize app-side serial processing, reset the line buffer. */
void app_serial_init(void);

/** @brief Wrapper around `tud_cdc_connected`. */
bool app_serial_is_connected(void);

/**
 * @brief Read one newline-terminated line from the RX buffer.
 *
 * This function is non-blocking. It accumulates characters internally and
 * returns only when a full line has been received. The returned line does not
 * contain the trailing newline character. It returns true if a full line was
 * copied into @p buffer.
 */
bool app_serial_read_line(char *buffer, size_t buffer_size);

/** @brief Write one newline-terminated line to the TX buffer. */
bool app_serial_write_line(char const *message);

#endif
