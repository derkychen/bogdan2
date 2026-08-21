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

static char   line_buf[SERIAL_READ_BUF_SIZE];
static size_t line_buf_current_size = 0u;

static void            line_buf_reset(void);
static serial_status_t line_buf_copy_to(serial_buf_t *buf);

void
serial_init (void)
{
    line_buf_reset();

    return;
}

serial_status_t
serial_read_line (serial_buf_t *buf)
{
    ASSERT(buf != NULL);
    ASSERT(buf->str != NULL);
    ASSERT(buf->size > 0u);

    if (tud_cdc_connected() == false)
    {
        line_buf_reset();

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
            return line_buf_copy_to(buf);
        }

        if (line_buf_current_size < (SERIAL_READ_BUF_SIZE - 1u))
        {
            line_buf[line_buf_current_size] = (char)byte;
            line_buf_current_size++;
            line_buf[line_buf_current_size] = '\0';
        }
        else
        {
            line_buf_reset();
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
line_buf_reset (void)
{
    line_buf_current_size = 0U;
    line_buf[0]           = '\0';

    return;
}

/** @brief Copy a line from the line buffer into @p buf. */
static serial_status_t
line_buf_copy_to (serial_buf_t *buf)
{
    if (line_buf_current_size >= buf->size)
    {
        line_buf_reset();
        return SERIAL_STATUS_ERR_DEST_BUFFER_OVERFLOW;
    }

    memcpy(buf->str, line_buf, line_buf_current_size);
    buf->str[line_buf_current_size] = '\0';

    line_buf_reset();

    return SERIAL_STATUS_OK_LINE_RECEIVED;
}
