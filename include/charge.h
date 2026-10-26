// charge.h — charging logic. Uses its own input/output structs so the team's
// bms_types.h is untouched; copy these fields into Inputs/Outputs later if wanted.
#pragma once
#include <Arduino.h>
#include "charge_config.h"

struct ChargeInputs {
  uint16_t cell_mv[NUM_STACK_DEVICES][CELLS_PER_DEVICE];  // for balancing decisions
  uint16_t min_cell_mv, max_cell_mv;
  float    pack_v;
  float    min_temp_c, max_temp_c;
  bool     cell_data_ok;        // readings fresh and valid
  float    pack_current_a;      // + = into the pack
  bool     sdc_closed;          // shutdown circuit closed / TS active (AIRs closed)
  bool     charger_connected;   // e.g. Elcon CC signal
  bool     charger_seen;        // Elcon status frame received recently (elcon_seen_within)
  uint8_t  charger_flags;       // Elcon status byte (0 = OK)
};

struct ChargeOutputs {
  float    charger_v_cmd;
  float    charger_i_cmd;
  bool     charger_stop;
  bool     balance_enable;      // false -> balancing must be off
  bool     balance_update;      // true -> (re)send balance_cells this cycle
  bool     balance_cells[NUM_STACK_DEVICES][CELLS_PER_DEVICE];
  uint8_t     charge_phase;     // ChargePhase
  const char* charge_abort_reason;
};

enum ChargePhase : uint8_t { CH_IDLE, CH_WAIT_CHARGER, CH_PRECHECK, CH_ACTIVE, CH_FINISHING, CH_DONE, CH_ABORTED };

void        charge_begin();                                     // start a charge session
void        charge_step(const ChargeInputs& in, ChargeOutputs& out); // logic only, call every tick
void        charge_apply(const ChargeOutputs& out);             // sends Elcon command, applies balancing
const char* charge_phase_name(uint8_t phase);