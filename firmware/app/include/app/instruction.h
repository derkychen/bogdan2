#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include <stdbool.h>

/**
 * @brief Pico-specific instructions received from the computer.
 *
 * This structure defines the grid to profile as well as some parameters
 * relevant to waveform capture. Note that this structure only contains the
 * instructions relevant to the Pico, and is not the full set of instructions
 * given to `bogdan2.sh`.
 */
typedef struct {
  /** Minimum coordinate on the x-axis in units. */
  int x_min;

  /** Maximum coordinate on the x-axis in units. */
  int x_max;

  /** The length of each unit on the x-axis in nanometres. */
  int x_unit_nm;

  /** Minimum coordinate on the x-axis in units. */
  int y_min;

  /** Maximum coordinate on the x-axis in units. */
  int y_max;

  /** The length of each unit on the x-axis in nanometres. */
  int y_unit_nm;

  /** The number of laser pulses that should be captured at each point.*/
  int num_pulses;

  /** The delay after the triggering of the PicoScope, in microseconds. */
  int posttrigger_time_us;
} Instruction;

/** @brief Initialise an instruction. */
void instruction_init(Instruction *inst, int x_min_unit, int x_max_unit,
                      int x_unit_nm, int y_min_unit, int y_max_unit,
                      int y_unit_nm, int num_pulses, int posttrigger_time_us);

/** @brief Parse the JSON instruction sent through serial. */
bool instruction_parse_json(const char *json, Instruction *inst);

#endif
