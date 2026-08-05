/**
 * @file serial.c
 * @brief Implementation of serial functionality.
 */
#include "app/serial.h"
#include "platform/samd21g18a/assert.h"
#include "tusb.h" // IWYU pragma: keep
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static char   line_buffer[SERIAL_READ_BUFFER_SIZE];
static size_t line_buffer_current_size = 0U;

static void line_buffer_reset(void);
static void line_buffer_copy_to(char *buffer, size_t buffer_size);

void
serial_init (void)
{
    line_buffer_reset();

    return;
}

serial_status_t
serial_read_line (char *buffer, size_t buffer_size)
{
    ASSERT(buffer != NULL);
    ASSERT(buffer_size > 0U);

    if (tud_cdc_connected() == false)
    {
        line_buffer_reset();

        return SERIAL_STATUS_ERR_DISCONNECTED;
    }

    while (tud_cdc_available() != 0u)
    {
        uint8_t byte;

        uint32_t count = tud_cdc_read(&byte, 1u);

        if (count != 1u)
        {
            return SERIAL_STATUS_OK_LINE_PENDING;
        }

        if (byte == '\r')
        {
            continue;
        }

        if (byte == '\n')
        {
            line_buffer_copy_to(buffer, buffer_size);
            return SERIAL_STATUS_OK_LINE_RECEIVED;
        }

        if (line_buffer_current_size < (SERIAL_READ_BUFFER_SIZE - 1u))
        {
            line_buffer[line_buffer_current_size] = (char)byte;
            line_buffer_current_size++;
            line_buffer[line_buffer_current_size] = '\0';
        }
        else
        {
            line_buffer_reset();
            return SERIAL_STATUS_ERR_LINE_BUFFER_OVERFLOW;
        }
    }

    return SERIAL_STATUS_OK_LINE_PENDING;
}

serial_status_t
serial_write_line (char const *message)
{
    ASSERT(message != NULL);

    if (tud_cdc_connected() == false)
    {
        return SERIAL_STATUS_ERR_DISCONNECTED;
    }

    size_t   message_size = strlen(message);
    uint32_t written      = tud_cdc_write(message, message_size);

    if (written != message_size)
    {
        tud_cdc_write_flush();
        return SERIAL_STATUS_ERR_LINE_WRITE_FAILED;
    }

    written = tud_cdc_write("\r\n", 2u);

    tud_cdc_write_flush();

    if (written == 2u)
    {
        return SERIAL_STATUS_OK;
    }

    return SERIAL_STATUS_ERR;
}

/** @brief Reset the line buffer. */
static void
line_buffer_reset (void)
{
    line_buffer_current_size = 0U;
    line_buffer[0]           = '\0';

    return;
}

/** @brief Copy a line from the line buffer into @p buffer. */
static void
line_buffer_copy_to (char *buffer, size_t buffer_size)
{
    size_t copy_size = line_buffer_current_size;

    if (copy_size > (buffer_size - 1U))
    {
        copy_size = buffer_size - 1U;
    }

    memcpy(buffer, line_buffer, copy_size);

    buffer[copy_size] = '\0';

    line_buffer_reset();
}
