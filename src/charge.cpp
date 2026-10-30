// charge.cpp
//
// Current request = min(cell limit, charger power/current limit, temp derate, taper),
// ramped up slowly. Decisions use the HIGHEST cell, not pack voltage.
// Balancing only while charging with the shutdown circuit closed (EV.8.3.3).
#include "charge.h"
#include "elcon.h"
#include "bq_init.h"

static ChargePhase phase = CH_IDLE;
static uint32_t    phase_ms = 0, last_bal_ms = 0, last_step_ms = 0;
static float       i_cmd = 0.0f;
static bool        bal_on = false;
static const char* abort_reason = "";

static const float PACK_TARGET_V = TOTAL_CELLS * (CELL_CHARGE_TARGET_MV / 1000.0f);

static void set_phase(ChargePhase p) { phase = p; phase_ms = millis(); }

static void abort_with(const char* why) {
  if (phase != CH_ABORTED) Serial.printf("[CHG] abort: %s\n", why);
  abort_reason = why;
  i_cmd = 0.0f;
  set_phase(CH_ABORTED);
}

// ------------------------------------------------------------ current limits

static float temp_limit_a(float tmin, float tmax) {
  if (tmin < CHARGE_TEMP_MIN_C || tmax > CHARGE_TEMP_MAX_C) return 0.0f;
  if (tmax <= CHARGE_DERATE_START_C) return CELL_CHARGE_I_MAX_A;
  float k = (CHARGE_TEMP_MAX_C - tmax) / (CHARGE_TEMP_MAX_C - CHARGE_DERATE_START_C);
  return CELL_CHARGE_I_MAX_A * k;
}

static float taper_limit_a(uint16_t max_mv) {
  if (max_mv <= CELL_TAPER_START_MV) return CELL_CHARGE_I_MAX_A;
  if (max_mv >= CELL_CHARGE_TARGET_MV) return 0.0f;
  float k = (float)(CELL_CHARGE_TARGET_MV - max_mv) / (CELL_CHARGE_TARGET_MV - CELL_TAPER_START_MV);
  return fmaxf(CELL_CHARGE_I_MAX_A * k, CHARGE_END_CURRENT_A * 0.5f);  // small floor to finish
}

static float charger_limit_a(float pack_v) {
  float p_lim = (pack_v > 1.0f) ? CHARGER_P_MAX_W / pack_v : CHARGER_I_MAX;
  return fminf(CHARGER_I_MAX, p_lim);
}

static bool charger_healthy(const ChargeInputs& in) { return in.charger_seen && in.charger_flags == 0; }

// ----------------------------------------------------------------- balancing

static void decide_balancing(const ChargeInputs& in, ChargeOutputs& out) {
  out.balance_update = false;
  bool allowed = (phase == CH_ACTIVE || phase == CH_DONE) && in.sdc_closed && in.cell_data_ok;
  out.balance_enable = allowed;
  if (!allowed) { bal_on = false; return; }

  if (bal_on && millis() - last_bal_ms < BALANCING_REEVALUATE_MS) return;
  last_bal_ms = millis();

  bool any = false;
  for (int d = 0; d < NUM_STACK_DEVICES; d++)
    for (int c = 0; c < CELLS_PER_DEVICE; c++) {
      uint16_t v = in.cell_mv[d][c];
      out.balance_cells[d][c] = v >= BALANCING_MIN_CELL_MV && v > in.min_cell_mv + BALANCING_THRESHOLD_MV;
      any |= out.balance_cells[d][c];
    }
  out.balance_update = true;   // apply: start if any cell selected, else stop
  bal_on = any;
}

// --------------------------------------------------------------------- API

void charge_begin() {
  i_cmd = 0.0f;
  bal_on = false;
  last_step_ms = 0;
  abort_reason = "";
  set_phase(CH_WAIT_CHARGER);
  if (PACK_TARGET_V > CHARGER_V_MAX) abort_with("pack target voltage above charger max (check cell count)");
}

void charge_step(const ChargeInputs& in, ChargeOutputs& out) {
  uint32_t now = millis();

  if (phase != CH_IDLE && phase != CH_ABORTED) {
    if (!in.cell_data_ok) abort_with("cell data invalid/stale");
    else if (!in.charger_connected && phase != CH_DONE && phase != CH_FINISHING)
      abort_with("charger disconnected");
  }

  switch (phase) {
    case CH_WAIT_CHARGER:
      if (in.charger_seen) set_phase(CH_PRECHECK);
      else if (now - phase_ms > ELCON_DETECT_TIMEOUT_MS) abort_with("no status from Elcon");
      break;

    case CH_PRECHECK: {
      bool temps_ok = in.min_temp_c >= CHARGE_TEMP_MIN_C && in.max_temp_c <= CHARGE_TEMP_MAX_C;
      bool not_full = in.max_cell_mv < CELL_CHARGE_TARGET_MV;
      bool window   = in.pack_v >= CHARGER_V_MIN * 0.95f;
      if (temps_ok && not_full && window && in.sdc_closed && charger_healthy(in)) {
        i_cmd = 0.0f;
        set_phase(CH_ACTIVE);
      } else if (now - phase_ms > 5000) {
        if (!temps_ok)           abort_with("temperature outside charge window");
        else if (!not_full)      abort_with("pack already full");
        else if (!window)        abort_with("pack below charger minimum voltage");
        else if (!in.sdc_closed) abort_with("shutdown circuit open / TS not active");
        else                     abort_with("Elcon not healthy");
      }
      break;
    }

    case CH_ACTIVE: {
      float target = CELL_CHARGE_I_MAX_A;
      target = fminf(target, charger_limit_a(in.pack_v));
      target = fminf(target, temp_limit_a(in.min_temp_c, in.max_temp_c));
      target = fminf(target, taper_limit_a(in.max_cell_mv));

      // Real elapsed time, so the ramp is right whatever rate the FSM calls us at.
      float dt = (last_step_ms ? (now - last_step_ms) : 0) / 1000.0f;
      if (dt > 1.0f) dt = 1.0f;
      if (target > i_cmd) i_cmd = fminf(target, i_cmd + CHARGE_RAMP_A_PER_S * dt);  // ramp up
      else                i_cmd = target;                                           // drop at once

      if (!in.sdc_closed)            abort_with("shutdown circuit opened");
      else if (!charger_healthy(in)) abort_with(in.charger_flags ? "Elcon status flag set" : "Elcon silent");
      else if (now - phase_ms > 10000 && in.pack_current_a > i_cmd + CHARGE_OVERCURRENT_MARGIN_A)
        abort_with("measured current above request");
      else if (in.max_cell_mv >= CELL_CHARGE_TARGET_MV && in.pack_current_a < CHARGE_END_CURRENT_A
               && now - phase_ms > 10000)
        set_phase(CH_FINISHING);   // highest cell at target and current tapered off
      break;
    }

    case CH_FINISHING:
      i_cmd = 0.0f;
      if (in.pack_current_a < 0.5f || now - phase_ms > 10000) set_phase(CH_DONE);
      break;

    default:   // IDLE, DONE (charger held at stop, balancing may continue), ABORTED
      break;
  }

  last_step_ms = now;

  // ---- Outputs ----
  bool charging = (phase == CH_ACTIVE);
  out.charger_v_cmd = PACK_TARGET_V;
  out.charger_i_cmd = charging ? i_cmd : 0.0f;
  out.charger_stop  = !charging;
  decide_balancing(in, out);
  out.charge_phase = phase;
  out.charge_abort_reason = abort_reason;
}

const char* charge_phase_name(uint8_t p) {
  switch (p) {
    case CH_IDLE:         return "IDLE";
    case CH_WAIT_CHARGER: return "WAIT_CHARGER";
    case CH_PRECHECK:     return "PRECHECK";
    case CH_ACTIVE:       return "ACTIVE";
    case CH_FINISHING:    return "FINISHING";
    case CH_DONE:         return "DONE";
    case CH_ABORTED:      return "ABORTED";
  }
  return "?";
}

// ------------------------------------------------------------------ apply

static uint32_t last_cmd_ms = 0;

void charge_apply(const ChargeOutputs& o) {
  // Elcon must hear from us at least every second (it stops after ~5 s of silence).
  if (millis() - last_cmd_ms >= ELCON_CMD_PERIOD_MS) {
    last_cmd_ms = millis();
    elcon_send(o.charger_v_cmd, o.charger_i_cmd, o.charger_stop);
  }

  if (!o.balance_enable) {
    if (bq_balancing_active()) bq_stop_balancing();
  } else if (o.balance_update) {
    bool any = false;
    for (int d = 0; d < NUM_STACK_DEVICES; d++)
      for (int c = 0; c < CELLS_PER_DEVICE; c++) any |= o.balance_cells[d][c];
    if (any) bq_start_balancing(o.balance_cells);
    else if (bq_balancing_active()) bq_stop_balancing();
  }
}