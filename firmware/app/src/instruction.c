#include "instruction.h"
#include "jsmn.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define MAX_JSON_TOKENS 64

void instruction_init(Instruction *inst, int x_min, int x_max,
                      int x_unit_nm, int y_min, int y_max,
                      int y_unit_nm, int num_pulses, int posttrigger_time_us) {
  inst->x_min = x_min;
  inst->x_max = x_max;
  inst->x_unit_nm = x_unit_nm;

  inst->y_min= y_min;
  inst->y_max= y_max;
  inst->y_unit_nm = y_unit_nm;

  inst->num_pulses = num_pulses;
  inst->posttrigger_time_us = posttrigger_time_us;
}

typedef enum {
  FIELD_INT,
  FIELD_STRING,
} FieldType;

typedef struct {
  const char *name;
  FieldType type;
  size_t offset;
  size_t size;
} FieldSpec;

#define FIELD_INT_SPEC(json_name, struct_type, member)                         \
  {json_name, FIELD_INT, offsetof(struct_type, member),                        \
   sizeof(((struct_type *)0)->member)}

#define FIELD_STRING_SPEC(json_name, struct_type, member)                      \
  {json_name, FIELD_STRING, offsetof(struct_type, member),                     \
   sizeof(((struct_type *)0)->member)}

static const FieldSpec instruction_fields[] = {
    FIELD_INT_SPEC("x_min", Instruction, x_min),
    FIELD_INT_SPEC("x_max", Instruction, x_max),
    FIELD_INT_SPEC("x_unit_nm", Instruction, x_unit_nm),

    FIELD_INT_SPEC("y_min", Instruction, y_min),
    FIELD_INT_SPEC("y_max", Instruction, y_max),
    FIELD_INT_SPEC("y_unit_nm", Instruction, y_unit_nm),

    FIELD_INT_SPEC("num_pulses", Instruction, num_pulses),
    FIELD_INT_SPEC("posttrigger_time_us", Instruction, posttrigger_time_us),
};

static const int instruction_field_count =
    sizeof(instruction_fields) / sizeof(instruction_fields[0]);

/**
 * @brief Check whether a jsmn token equals a C string.
 */
static bool token_streq(const char *json, const jsmntok_t *tok, const char *s) {
  int token_len = tok->end - tok->start;
  int string_len = strlen(s);

  if (tok->type != JSMN_STRING) {
    return false;
  }

  if (token_len != string_len) {
    return false;
  }

  return strncmp(json + tok->start, s, token_len) == 0;
}

/**
 * @brief Convert a jsmn token to an integer.
 */
static int token_to_int(const char *json, const jsmntok_t *tok) {
  char temp[24];
  int len = tok->end - tok->start;

  if (len < 0) {
    return 0;
  }

  if (len >= (int)sizeof(temp)) {
    len = sizeof(temp) - 1;
  }

  memcpy(temp, json + tok->start, len);
  temp[len] = '\0';

  return atoi(temp);
}

static bool set_field_from_token(const char *json, const jsmntok_t *value,
                                 void *base, const FieldSpec *field) {
  char *target = (char *)base + field->offset;

  switch (field->type) {
  case FIELD_INT:
    if (value->type != JSMN_PRIMITIVE) {
      return false;
    }

    *(int *)target = token_to_int(json, value);
    return true;

  case FIELD_STRING: {
    if (value->type != JSMN_STRING) {
      return false;
    }

    int len = value->end - value->start;

    if (len < 0) {
      return false;
    }

    if ((size_t)len >= field->size) {
      len = field->size - 1;
    }

    memcpy(target, json + value->start, len);
    target[len] = '\0';
    return true;
  }

  default:
    return false;
  }
}

bool instruction_parse_json(const char *json, Instruction *inst) {
  jsmn_parser parser;
  jsmntok_t tokens[MAX_JSON_TOKENS];

  if (json == NULL || inst == NULL) {
    return false;
  }

  jsmn_init(&parser);

  int token_count =
      jsmn_parse(&parser, json, strlen(json), tokens, MAX_JSON_TOKENS);

  if (token_count < 0) {
    return false;
  }

  if (token_count < 1 || tokens[0].type != JSMN_OBJECT) {
    return false;
  }

  Instruction temp = *inst;

  int i = 1;

  for (int pair = 0; pair < tokens[0].size; pair++) {
    jsmntok_t *key = &tokens[i];
    jsmntok_t *value = &tokens[i + 1];

    for (int f = 0; f < instruction_field_count; f++) {
      const FieldSpec *field = &instruction_fields[f];

      if (token_streq(json, key, field->name)) {
        if (!set_field_from_token(json, value, &temp, field)) {
          return false;
        }

        break;
      }
    }

    i += 2;
  }

  *inst = temp;
  return true;
}
