#include "app/instruction.h"
#include "jsmn.h"
#include "platform/samd21g18a/assert.h"
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MAX_JSON_TOKENS (64U)

#define FIELD_MASK_X_MIN               (1UL << 0U)
#define FIELD_MASK_X_MAX               (1UL << 1U)
#define FIELD_MASK_X_UNIT_NM           (1UL << 2U)
#define FIELD_MASK_X_ORIGIN_NM         (1UL << 3U)
#define FIELD_MASK_Y_MIN               (1UL << 4U)
#define FIELD_MASK_Y_MAX               (1UL << 5U)
#define FIELD_MASK_Y_UNIT_NM           (1UL << 6U)
#define FIELD_MASK_Y_ORIGIN_NM         (1UL << 7U)
#define FIELD_MASK_NUM_PULSES          (1UL << 8U)
#define FIELD_MASK_POSTTRIGGER_TIME_US (1UL << 9U)

#define FIELD_REQUIRED_MASK                                                  \
    (FIELD_MASK_X_MIN | FIELD_MASK_X_MAX | FIELD_MASK_X_UNIT_NM              \
     | FIELD_MASK_X_ORIGIN_NM | FIELD_MASK_Y_MIN | FIELD_MASK_Y_MAX          \
     | FIELD_MASK_Y_UNIT_NM | FIELD_MASK_Y_ORIGIN_NM | FIELD_MASK_NUM_PULSES \
     | FIELD_MASK_POSTTRIGGER_TIME_US)

/** @brief Parsing status codes. */
typedef enum
{
    PARSE_STATUS_OK = 0,
    PARSE_STATUS_ERR,
} parse_status_t;

/** @brief Field getting and setting status codes. */
typedef enum
{
    FIELD_STATUS_OK = 0,
    FIELD_STATUS_ERR_GET,
    FIELD_STATUS_ERR_SET,
} field_status_t;

/** @brief Enumeration of field types. */
typedef enum
{
    FIELD_TYPE_INT = 0,
    FIELD_TYPE_UINT32,
} field_type_t;

/**
 * @brief JSON fields.
 *
 * This format is used to store the internal schema for instructions.
 */
typedef struct
{
    /** Name. */
    char const *name;

    /** Type. */
    field_type_t type;

    /** Offset in the instructions structure. */
    size_t offset;

    /** Mask specific to the field. */
    uint32_t mask;
} field_spec_t;

/** @brief JSON token structure. */
typedef struct
{
    /** JSMN token data. */
    jsmntok_t const *data;

    /** The JSON string. */
    char const *json;
} token_t;

static field_spec_t const instruction_fields[] = {
    {
        .name   = "x_min",
        .type   = FIELD_TYPE_INT,
        .offset = offsetof(app_instruction_t, x_min),
        .mask   = FIELD_MASK_X_MIN,
    },
    {
        .name   = "x_max",
        .type   = FIELD_TYPE_INT,
        .offset = offsetof(app_instruction_t, x_max),
        .mask   = FIELD_MASK_X_MAX,
    },
    {
        .name   = "x_unit_nm",
        .type   = FIELD_TYPE_UINT32,
        .offset = offsetof(app_instruction_t, x_unit_nm),
        .mask   = FIELD_MASK_X_UNIT_NM,
    },
    {
        .name   = "x_origin_nm",
        .type   = FIELD_TYPE_INT,
        .offset = offsetof(app_instruction_t, x_origin_nm),
        .mask   = FIELD_MASK_X_ORIGIN_NM,
    },
    {
        .name   = "y_min",
        .type   = FIELD_TYPE_INT,
        .offset = offsetof(app_instruction_t, y_min),
        .mask   = FIELD_MASK_Y_MIN,
    },
    {
        .name   = "y_max",
        .type   = FIELD_TYPE_INT,
        .offset = offsetof(app_instruction_t, y_max),
        .mask   = FIELD_MASK_Y_MAX,
    },
    {
        .name   = "y_unit_nm",
        .type   = FIELD_TYPE_UINT32,
        .offset = offsetof(app_instruction_t, y_unit_nm),
        .mask   = FIELD_MASK_Y_UNIT_NM,
    },
    {
        .name   = "y_origin_nm",
        .type   = FIELD_TYPE_INT,
        .offset = offsetof(app_instruction_t, y_origin_nm),
        .mask   = FIELD_MASK_Y_ORIGIN_NM,
    },
    {
        .name   = "num_pulses",
        .type   = FIELD_TYPE_UINT32,
        .offset = offsetof(app_instruction_t, num_pulses),
        .mask   = FIELD_MASK_NUM_PULSES,
    },
    {
        .name   = "posttrigger_time_us",
        .type   = FIELD_TYPE_UINT32,
        .offset = offsetof(app_instruction_t, posttrigger_time_us),
        .mask   = FIELD_MASK_POSTTRIGGER_TIME_US,
    },
};

/** Convert a parsing status code to a field status code. */
static field_status_t
parse_to_field_status (parse_status_t status)
{
    switch (status)
    {
        case PARSE_STATUS_OK:
            return FIELD_STATUS_OK;
        case PARSE_STATUS_ERR:
        default:
            return FIELD_STATUS_ERR_SET;
    }
}

/** @brief Check that a token is valid. */
static bool
token_is_valid (token_t const *token)
{
    PLATFORM_SAMD21G18A_ASSERT(token != NULL);
    PLATFORM_SAMD21G18A_ASSERT(token->data != NULL);
    PLATFORM_SAMD21G18A_ASSERT(token->json != NULL);

    // Check for garbage token data.
    if ((token->data) == NULL || (token->data->start < 0)
        || (token->data->end < token->data->start))
    {
        return false;
    }

    return true;
}

/** @brief Check whether a token's text equals a string. */
static bool
token_text_equals_str (token_t const *token, char const *string)
{
    size_t token_length;
    size_t string_length;

    if (!token_is_valid(token))
    {
        return false;
    }

    if (token->data->type != JSMN_STRING)
    {
        return false;
    }

    token_length  = (size_t)(token->data->end - token->data->start);
    string_length = strlen(string);

    if (token_length != string_length)
    {
        return false;
    }

    return (strncmp(&token->json[token->data->start], string, token_length)
            == 0);
}

/** @brief Copy a token into a buffer. */
static void
token_copy (token_t const *token, char *buffer, size_t buffer_size)
{
    size_t token_length;

    PLATFORM_SAMD21G18A_ASSERT(buffer != NULL);
    PLATFORM_SAMD21G18A_ASSERT(buffer_size > 0U);

    if (!token_is_valid(token))
    {
        return;
    }

    token_length = (size_t)(token->data->end - token->data->start);

    if (token_length >= buffer_size)
    {
        return;
    }

    memcpy(buffer, &token->json[token->data->start], token_length);
    buffer[token_length] = '\0';

    return;
}

/** @brief Parse a token into an integer. */
static parse_status_t
token_parse_int (token_t const *token, int *value)
{
    char    buffer[24];
    char   *end;
    int64_t parsed;

    PLATFORM_SAMD21G18A_ASSERT(token != NULL);
    PLATFORM_SAMD21G18A_ASSERT(value != NULL);

    if (token->data->type != JSMN_PRIMITIVE)
    {
        return PARSE_STATUS_ERR;
    }

    token_copy(token, buffer, sizeof(buffer));

    errno  = 0;
    parsed = strtol(buffer, &end, 10);

    if ((errno != 0) || (*end != '\0'))
    {
        return PARSE_STATUS_ERR;
    }

    if ((parsed < (int64_t)INT_MIN) || (parsed > (int64_t)INT_MAX))
    {
        return PARSE_STATUS_ERR;
    }

    *value = (int)parsed;

    return PARSE_STATUS_OK;
}

/** @brief Parse a token into a `uint32_t`. */
static parse_status_t
token_parse_uint32 (token_t const *token, uint32_t *value)
{
    char          buffer[24];
    char         *end;
    unsigned long parsed;

    PLATFORM_SAMD21G18A_ASSERT(token != NULL);
    PLATFORM_SAMD21G18A_ASSERT(value != NULL);

    if (token->data->type != JSMN_PRIMITIVE)
    {
        return PARSE_STATUS_ERR;
    }

    token_copy(token, buffer, sizeof(buffer));

    if (buffer[0] == '-')
    {
        return PARSE_STATUS_ERR;
    }

    errno  = 0;
    parsed = strtoul(buffer, &end, 10);

    if ((errno != 0) || (*end != '\0'))
    {
        return PARSE_STATUS_ERR;
    }

    if (parsed > (unsigned long)UINT32_MAX)
    {
        return PARSE_STATUS_ERR;
    }

    *value = (uint32_t)parsed;

    return PARSE_STATUS_OK;
}

static field_status_t
token_field_find (token_t const *token, field_spec_t const **spec)
{
    size_t index;

    for (index = 0U;
         index < (sizeof(instruction_fields) / sizeof(instruction_fields[0]));
         index++)
    {
        if (token_text_equals_str(token, instruction_fields[index].name))
        {
            *spec = &instruction_fields[index];
        }
    }

    return FIELD_STATUS_ERR_GET;
}

static field_status_t
token_field_set (token_t const      *token,
                 app_instruction_t  *instruction,
                 field_spec_t const *field)
{
    char *target;

    if ((instruction == NULL) || (field == NULL))
    {
        return false;
    }

    target = ((char *)instruction) + field->offset;

    switch (field->type)
    {
        case FIELD_TYPE_INT:
            return parse_to_field_status(token_parse_int(token, (int *)target));

        case FIELD_TYPE_UINT32:
            return parse_to_field_status(
                token_parse_uint32(token, (uint32_t *)target));

        default:
            return FIELD_STATUS_ERR_SET;
    }
}

app_instruction_status_t
app_instruction_parse_json (app_instruction_t *instruction, char const *json)
{
    jsmn_parser         parser;
    token_t             token;
    jsmntok_t           tokens_data[MAX_JSON_TOKENS];
    app_instruction_t   temp;
    field_spec_t const *field;
    uint32_t            fields_seen;
    int                 token_count;
    int                 token_index;

    PLATFORM_SAMD21G18A_ASSERT(instruction != NULL);
    PLATFORM_SAMD21G18A_ASSERT(json != NULL);

    jsmn_init(&parser);

    token_count = jsmn_parse(&parser,
                             json,
                             strlen(json),
                             tokens_data,
                             (unsigned int)MAX_JSON_TOKENS);

    if (token_count < 0)
    {
        return APP_INSTRUCTION_STATUS_ERR_JSON_PARSE;
    }

    if ((token_count < 1) || (tokens_data[0].type != JSMN_OBJECT))
    {
        return APP_INSTRUCTION_STATUS_ERR_JSON_PARSE;
    }

    temp        = *instruction;
    fields_seen = 0U;
    token_index = 1;

    for (size_t pair_index = 0; pair_index < (size_t)tokens_data[0].size;
         pair_index++)
    {
        if ((token_index + 1) >= token_count)
        {
            return false;
        }

        token = (token_t) {
            .data = &tokens_data[token_index],
            .json = json,
        };

        if (token_field_find(&token, &field) != FIELD_STATUS_OK)
        {
            return APP_INSTRUCTION_STATUS_ERR_JSON_PARSE;
        }

        // Unknown fields are ignored.
        token = (token_t) {
            .data = &tokens_data[token_index + 1],
            .json = json,
        };

        if (field != NULL)
        {
            if (token_field_set(&token, &temp, field) != FIELD_STATUS_OK)
            {
                return APP_INSTRUCTION_STATUS_ERR_JSON_PARSE;
            }

            fields_seen |= field->mask;
        }

        token_index += 2;
    }

    if ((fields_seen & FIELD_REQUIRED_MASK) != FIELD_REQUIRED_MASK)
    {
        return APP_INSTRUCTION_STATUS_ERR_JSON_MISSING_REQUIRED_FIELDS;
    }

    *instruction = temp;

    return APP_INSTRUCTION_STATUS_OK_PARSED;
}
