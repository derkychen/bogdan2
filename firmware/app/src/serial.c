#include "app/serial.h"
#include "platform/samd21g18a/assert.h"
#include "tusb.h" // IWYU pragma: keep
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define READ_CHUNK_SIZE (64U)

static char   line_buffer[APP_SERIAL_READ_BUFFER_SIZE];
static size_t line_buffer_current_size = 0U;

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
    size_t copy_size;

    copy_size = line_buffer_current_size;

    if (copy_size > (buffer_size - 1U))
    {
        copy_size = buffer_size - 1U;
    }

    memcpy(buffer, line_buffer, copy_size);
    buffer[copy_size] = '\0';

    line_buffer_reset();
}

void
app_serial_init (void)
{
    line_buffer_reset();

    return;
}

bool
app_serial_connected (void)
{
    return tud_cdc_connected();
}

app_serial_status_t
app_serial_read_line (char *buffer, size_t buffer_size)
{
    uint8_t  byte;
    uint32_t count;

    PLATFORM_SAMD21G18A_ASSERT(buffer != NULL);
    PLATFORM_SAMD21G18A_ASSERT(buffer_size > 0U);

    if (tud_cdc_connected() == false)
    {
        line_buffer_reset();
        return APP_SERIAL_STATUS_ERR_DISCONNECTED;
    }

    while (tud_cdc_available() != 0u)
    {
        count = tud_cdc_read(&byte, 1u);

        if (count != 1u)
        {
            return APP_SERIAL_STATUS_OK_LINE_PENDING;
        }

        if (byte == '\r')
        {
            continue;
        }

        if (byte == '\n')
        {
            line_buffer_copy_to(buffer, buffer_size);
            return APP_SERIAL_STATUS_OK_LINE_RECEIVED;
        }

        if (line_buffer_current_size < (APP_SERIAL_READ_BUFFER_SIZE - 1u))
        {
            line_buffer[line_buffer_current_size] = (char)byte;
            line_buffer_current_size++;
            line_buffer[line_buffer_current_size] = '\0';
        }
        else
        {
            line_buffer_reset();
            return APP_SERIAL_STATUS_ERR_LINE_BUFFER_OVERFLOW;
        }
    }

    return false;
}

app_serial_status_t
app_serial_write_line (char const *message)
{
    size_t   message_size;
    uint32_t written;

    PLATFORM_SAMD21G18A_ASSERT(message != NULL);

    if (tud_cdc_connected() == false)
    {
        return APP_SERIAL_STATUS_ERR_DISCONNECTED;
    }

    message_size = strlen(message);

    written = tud_cdc_write(message, message_size);

    if (written != message_size)
    {
        tud_cdc_write_flush();
        return APP_SERIAL_STATUS_ERR_LINE_WRITE_FAILED;
    }

    written = tud_cdc_write("\r\n", 2u);

    tud_cdc_write_flush();

    if (written == 2u)
    {
        return APP_SERIAL_STATUS_OK;
    }

    return APP_SERIAL_STATUS_ERR;
}
