#include "app/serial.h"
#include "class/cdc/cdc_device.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define READ_CHUNK_SIZE (64u)

static char   line_buffer[APP_SERIAL_READ_BUFFER_SIZE];
static size_t line_buffer_current_size = 0u;

/** @brief Reset the line buffer. */
static void
line_buffer_reset (void)
{
    line_buffer_current_size = 0u;
    line_buffer[0]           = '\0';

    return;
}

/** @brief Copy a line from the line buffer into @p buffer. */
static bool
line_buffer_copy_to (char *buffer, size_t buffer_size)
{
    size_t copy_size;

    if (buffer == NULL)
    {
        line_buffer_reset();
        return false;
    }

    if (buffer_size == 0u)
    {
        line_buffer_reset();
        return false;
    }

    copy_size = line_buffer_current_size;

    if (copy_size > (buffer_size - 1u))
    {
        copy_size = buffer_size - 1u;
    }

    memcpy(buffer, line_buffer, copy_size);
    buffer[copy_size] = '\0';

    line_buffer_reset();

    return true;
}

void
app_serial_init (void)
{
    line_buffer_reset();

    return;
}

bool
app_serial_is_connected (void)
{
    return tud_cdc_connected();
}

bool
app_serial_read_line (char *buffer, size_t buffer_size)
{
    uint8_t  byte;
    uint32_t count;

    if (tud_cdc_connected() == false)
    {
        line_buffer_reset();
        return false;
    }

    while (tud_cdc_available() != 0u)
    {
        count = tud_cdc_read(&byte, 1u);

        if (count != 1u)
        {
            return false;
        }

        if (byte == '\r')
        {
            continue;
        }

        if (byte == '\n')
        {
            return line_buffer_copy_to(buffer, buffer_size);
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
            return false;
        }
    }

    return false;
}

bool
app_serial_write_line (char const *message)
{
    size_t   message_size;
    uint32_t written;

    if (message == NULL)
    {
        return false;
    }

    if (tud_cdc_connected() == false)
    {
        return false;
    }

    message_size = strlen(message);

    written = tud_cdc_write(message, message_size);

    if (written != message_size)
    {
        tud_cdc_write_flush();
        return false;
    }

    written = tud_cdc_write("\r\n", 2u);

    tud_cdc_write_flush();

    return (written == 2u);
}
