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

#define ARRAY_COUNT(array) (sizeof(array) / sizeof((array)[0]))

#define INSTRUCTION_MODE_MASK(mode) (UINT32_C(1) << (uint32_t)(mode))

#define FIELD_REQUIRED_MODES_ALL                              \
    (INSTRUCTION_MODE_MASK(APP_INSTRUCTION_MODE_POINT_COUNT)  \
     | INSTRUCTION_MODE_MASK(APP_INSTRUCTION_MODE_POINT_TIME) \
     | INSTRUCTION_MODE_MASK(APP_INSTRUCTION_MODE_CONTINUOUS))

#define FIELD_REQUIRED_MODES_POINT_COUNT \
    INSTRUCTION_MODE_MASK(APP_INSTRUCTION_MODE_POINT_COUNT)

#define FIELD_REQUIRED_MODES_POINT_TIME \
    INSTRUCTION_MODE_MASK(APP_INSTRUCTION_MODE_POINT_TIME)

#define INSTRUCTION_FIELD_LIST(X)                                     \
    X(MODE, "mode", mode, MODE, ALL)                                  \
    X(X_MIN, "x_min", x_min, INT, ALL)                                \
    X(X_MAX, "x_max", x_max, INT, ALL)                                \
    X(X_UNIT_NM, "x_unit_nm", x_unit_nm, UINT32, ALL)                 \
    X(X_ORIGIN_NM, "x_origin_nm", x_origin_nm, INT, ALL)              \
    X(Y_MIN, "y_min", y_min, INT, ALL)                                \
    X(Y_MAX, "y_max", y_max, INT, ALL)                                \
    X(Y_UNIT_NM, "y_unit_nm", y_unit_nm, UINT32, ALL)                 \
    X(Y_ORIGIN_NM, "y_origin_nm", y_origin_nm, INT, ALL)              \
    X(NUM_PULSES, "num_pulses", num_pulses, UINT32, POINT_COUNT)      \
    X(WAIT_TIME_US, "wait_time_ms", wait_time_ms, UINT32, POINT_TIME) \
    X(POSTTRIGGER_TIME_US,                                            \
      "posttrigger_time_us",                                          \
      posttrigger_time_us,                                            \
      UINT32,                                                         \
      POINT_COUNT)

#define INSTRUCTION_MODE_LIST(X)                       \
    X("point_count", APP_INSTRUCTION_MODE_POINT_COUNT) \
    X("point_time", APP_INSTRUCTION_MODE_POINT_TIME)   \
    X("continuous", APP_INSTRUCTION_MODE_CONTINUOUS)

/** @brief Parsing status codes. */
typedef enum
{
    PARSE_STATUS_OK = 0,
    PARSE_STATUS_ERR,
} parse_status_t;

/** @brief Enumeration of field types. */
typedef enum
{
    FIELD_TYPE_MODE = 0,
    FIELD_TYPE_INT,
    FIELD_TYPE_UINT32,
} field_type_t;

#define FIELD_INDEX_ENUM(id, json_name, member, field_type, requirement) \
    FIELD_INDEX_##id,

typedef enum
{
    INSTRUCTION_FIELD_LIST(FIELD_INDEX_ENUM)

        FIELD_INDEX_COUNT,
} field_index_t;

#undef FIELD_INDEX_ENUM

#define FIELD_MASK(id) (UINT32_C(1) << FIELD_INDEX_##id)

_Static_assert(FIELD_INDEX_COUNT <= 32, "Too many instruction fields");
_Static_assert(APP_INSTRUCTION_MODE_COUNT <= 32, "Too many instruction modes");

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

    /** Modes for which this field is required. */
    uint32_t required_modes;
} field_spec_t;

/** @brief JSON token structure. */
typedef struct
{
    /** JSMN token data. */
    jsmntok_t const *data;

    /** The JSON string. */
    char const *json;
} token_t;

#define FIELD_SPEC_INITIALIZER(id, json_name, member, field_type, requirement) \
    {                                                                          \
        .name           = json_name,                                           \
        .type           = FIELD_TYPE_##field_type,                             \
        .offset         = offsetof(app_instruction_t, member),                 \
        .mask           = FIELD_MASK(id),                                      \
        .required_modes = FIELD_REQUIRED_MODES_##requirement,                  \
    },

static field_spec_t const instruction_fields[]
    = { INSTRUCTION_FIELD_LIST(FIELD_SPEC_INITIALIZER) };

#undef FIELD_SPEC_INITIALIZER

_Static_assert(ARRAY_COUNT(instruction_fields) == FIELD_INDEX_COUNT,
               "Instruction field table is incomplete");

/** @brief Check that a token is valid. */
static bool
token_is_valid (token_t const *token)
{
    PLATFORM_SAMD21G18A_ASSERT(token != NULL);

    if (token == NULL)
    {
        return false;
    }

    PLATFORM_SAMD21G18A_ASSERT(token->data != NULL);
    PLATFORM_SAMD21G18A_ASSERT(token->json != NULL);

    // Check for garbage token data.
    if ((token->data == NULL) || (token->json == NULL)
        || (token->data->start < 0) || (token->data->end < token->data->start))
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

    PLATFORM_SAMD21G18A_ASSERT(string != NULL);

    if (!token_is_valid(token) || (string == NULL))
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
static parse_status_t
token_copy (token_t const *token, char *buffer, size_t buffer_size)
{
    size_t token_length;

    PLATFORM_SAMD21G18A_ASSERT(buffer != NULL);
    PLATFORM_SAMD21G18A_ASSERT(buffer_size > 0U);

    if (!token_is_valid(token) || (buffer == NULL) || (buffer_size == 0U))
    {
        return PARSE_STATUS_ERR;
    }

    token_length = (size_t)(token->data->end - token->data->start);

    if (token_length >= buffer_size)
    {
        return PARSE_STATUS_ERR;
    }

    memcpy(buffer, &token->json[token->data->start], token_length);
    buffer[token_length] = '\0';

    return PARSE_STATUS_OK;
}

/** @brief Parse a token into a mode. */
static parse_status_t
token_parse_mode (token_t const *token, app_instruction_mode_t *value)
{
    PLATFORM_SAMD21G18A_ASSERT(token != NULL);
    PLATFORM_SAMD21G18A_ASSERT(value != NULL);

    if (!token_is_valid(token) || (value == NULL)
        || (token->data->type != JSMN_STRING))
    {
        return PARSE_STATUS_ERR;
    }

#define TOKEN_PARSE_MODE(json_name, mode_value)  \
    if (token_text_equals_str(token, json_name)) \
    {                                            \
        *value = mode_value;                     \
        return PARSE_STATUS_OK;                  \
    }

    INSTRUCTION_MODE_LIST(TOKEN_PARSE_MODE)

#undef TOKEN_PARSE_MODE

    return PARSE_STATUS_ERR;
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

    if (!token_is_valid(token) || (value == NULL)
        || (token->data->type != JSMN_PRIMITIVE))
    {
        return PARSE_STATUS_ERR;
    }

    if (token_copy(token, buffer, sizeof(buffer)) != PARSE_STATUS_OK)
    {
        return PARSE_STATUS_ERR;
    }

    errno  = 0;
    parsed = strtol(buffer, &end, 10);

    if ((errno != 0) || (end == buffer) || (*end != '\0'))
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

    if (!token_is_valid(token) || (value == NULL)
        || (token->data->type != JSMN_PRIMITIVE))
    {
        return PARSE_STATUS_ERR;
    }

    if (token_copy(token, buffer, sizeof(buffer)) != PARSE_STATUS_OK)
    {
        return PARSE_STATUS_ERR;
    }

    if (buffer[0] == '-')
    {
        return PARSE_STATUS_ERR;
    }

    errno  = 0;
    parsed = strtoul(buffer, &end, 10);

    if ((errno != 0) || (end == buffer) || (*end != '\0'))
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

static field_spec_t const *
token_field_find (token_t const *token)
{
    size_t index;

    PLATFORM_SAMD21G18A_ASSERT(token != NULL);

    for (index = 0U; index < ARRAY_COUNT(instruction_fields); index++)
    {
        if (token_text_equals_str(token, instruction_fields[index].name))
        {
            return &instruction_fields[index];
        }
    }

    return NULL;
}

static parse_status_t
token_field_set (token_t const      *token,
                 app_instruction_t  *instruction,
                 field_spec_t const *field)
{
    char *target;

    PLATFORM_SAMD21G18A_ASSERT(instruction != NULL);
    PLATFORM_SAMD21G18A_ASSERT(field != NULL);

    if ((instruction == NULL) || (field == NULL))
    {
        return PARSE_STATUS_ERR;
    }

    target = ((char *)instruction) + field->offset;

    switch (field->type)
    {
        case FIELD_TYPE_MODE:
            return token_parse_mode(token, (app_instruction_mode_t *)target);

        case FIELD_TYPE_INT:
            return token_parse_int(token, (int *)target);

        case FIELD_TYPE_UINT32:
            return token_parse_uint32(token, (uint32_t *)target);

        default:
            return PARSE_STATUS_ERR;
    }
}

/** @brief Get the required field mask for an instruction mode. */
static uint32_t
instruction_required_mask_get (app_instruction_mode_t mode)
{
    uint32_t mode_mask;
    uint32_t required_mask;

    PLATFORM_SAMD21G18A_ASSERT((uint32_t)mode
                               < (uint32_t)APP_INSTRUCTION_MODE_COUNT);

    if ((uint32_t)mode >= (uint32_t)APP_INSTRUCTION_MODE_COUNT)
    {
        return 0U;
    }

    mode_mask     = INSTRUCTION_MODE_MASK(mode);
    required_mask = 0U;

    for (size_t index = 0U; index < ARRAY_COUNT(instruction_fields); index++)
    {
        if ((instruction_fields[index].required_modes & mode_mask) != 0U)
        {
            required_mask |= instruction_fields[index].mask;
        }
    }

    return required_mask;
}

/** @brief Get the index immediately after a token and its children. */
static parse_status_t
token_next_index_get (jsmntok_t const *tokens_data,
                      int              token_count,
                      int              token_index,
                      int             *next_token_index)
{
    int index;
    int token_end;

    PLATFORM_SAMD21G18A_ASSERT(tokens_data != NULL);
    PLATFORM_SAMD21G18A_ASSERT(next_token_index != NULL);

    if ((tokens_data == NULL) || (next_token_index == NULL) || (token_index < 0)
        || (token_index >= token_count))
    {
        return PARSE_STATUS_ERR;
    }

    if ((tokens_data[token_index].start < 0)
        || (tokens_data[token_index].end < tokens_data[token_index].start))
    {
        return PARSE_STATUS_ERR;
    }

    token_end = tokens_data[token_index].end;
    index     = token_index + 1;

    while ((index < token_count) && (tokens_data[index].start < token_end))
    {
        if ((tokens_data[index].start < 0)
            || (tokens_data[index].end < tokens_data[index].start))
        {
            return PARSE_STATUS_ERR;
        }

        index++;
    }

    *next_token_index = index;

    return PARSE_STATUS_OK;
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
    uint32_t            required_fields;
    int                 token_count;
    int                 token_index;
    int                 next_token_index;

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

    temp        = (app_instruction_t) { 0 };
    fields_seen = 0U;
    token_index = 1;

    while ((token_index < token_count)
           && (tokens_data[token_index].start < tokens_data[0].end))
    {
        token = (token_t) {
            .data = &tokens_data[token_index],
            .json = json,
        };

        if (!token_is_valid(&token) || (token.data->type != JSMN_STRING))
        {
            return APP_INSTRUCTION_STATUS_ERR_JSON_PARSE;
        }

        field = token_field_find(&token);
        token_index++;

        if ((token_index >= token_count)
            || (tokens_data[token_index].start >= tokens_data[0].end))
        {
            return APP_INSTRUCTION_STATUS_ERR_JSON_PARSE;
        }

        token = (token_t) {
            .data = &tokens_data[token_index],
            .json = json,
        };

        if (field != NULL)
        {
            if (token_field_set(&token, &temp, field) != PARSE_STATUS_OK)
            {
                return APP_INSTRUCTION_STATUS_ERR_JSON_PARSE;
            }

            fields_seen |= field->mask;
        }

        if (token_next_index_get(
                tokens_data, token_count, token_index, &next_token_index)
            != PARSE_STATUS_OK)
        {
            return APP_INSTRUCTION_STATUS_ERR_JSON_PARSE;
        }

        token_index = next_token_index;
    }

    if ((fields_seen & FIELD_MASK(MODE)) == 0U)
    {
        return APP_INSTRUCTION_STATUS_ERR_JSON_MISSING_REQUIRED_FIELDS;
    }

    if ((uint32_t)temp.mode >= (uint32_t)APP_INSTRUCTION_MODE_COUNT)
    {
        return APP_INSTRUCTION_STATUS_ERR_JSON_PARSE;
    }

    required_fields = instruction_required_mask_get(temp.mode);

    if ((fields_seen & required_fields) != required_fields)
    {
        return APP_INSTRUCTION_STATUS_ERR_JSON_MISSING_REQUIRED_FIELDS;
    }

    *instruction = temp;

    return APP_INSTRUCTION_STATUS_OK_PARSED;
}
