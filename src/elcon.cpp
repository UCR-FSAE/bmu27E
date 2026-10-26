// elcon.cpp
// BMS -> charger 0x1806E5F4: [V_H][V_L][I_H][I_L][ctrl][0][0][0], 0.1 V / 0.1 A per bit, ctrl 0=charge 1=stop
// Charger -> BMS 0x18FF50E5: [V_H][V_L][I_H][I_L][flags][..]
// Confirm byte/bit layout against the HK-LF-312-20 CAN spec from EV West.
#include "elcon.h"
#include "charge_config.h"

ElconStatus g_elcon;
static ESP32_CAN* bus = nullptr;

void elcon_attach(ESP32_CAN& b) { bus = &b; }
void elcon_reset() { memset(&g_elcon, 0, sizeof(g_elcon)); }

void elcon_send(float volts, float amps, bool stop) {
  if (!bus) return;
  if (volts > CHARGER_V_MAX) volts = CHARGER_V_MAX;
  if (volts < 0) volts = 0;
  if (amps > CHARGER_I_MAX) amps = CHARGER_I_MAX;
  if (amps < 0 || stop) amps = 0;

  uint16_t v = (uint16_t)(volts * 10.0f + 0.5f);
  uint16_t i = (uint16_t)(amps * 10.0f + 0.5f);
  CAN_message_t m = {};
  m.id = ELCON_CMD_ID;
  m.flags.extended = true;     // 29-bit ID — required, the Elcon ignores standard frames
  m.len = 8;
  m.buf[0] = v >> 8; m.buf[1] = v & 0xFF;
  m.buf[2] = i >> 8; m.buf[3] = i & 0xFF;
  m.buf[4] = stop ? 1 : 0;
  bus->write(m);
}

bool elcon_handle(const CAN_message_t& m) {
  // Match on ID only (a 29-bit ID this large can't be a standard frame), so this
  // still works if read() doesn't fill in flags.extended.
  if (m.id != ELCON_STATUS_ID || m.len < 5) return false;
  g_elcon.out_v = ((m.buf[0] << 8) | m.buf[1]) / 10.0f;
  g_elcon.out_a = ((m.buf[2] << 8) | m.buf[3]) / 10.0f;
  g_elcon.flags = m.buf[4];
  g_elcon.last_seen_ms = millis();
  return true;
}

bool elcon_seen_within(uint32_t ms) {
  return g_elcon.last_seen_ms != 0 && (millis() - g_elcon.last_seen_ms) < ms;
}

bool elcon_healthy() {
  return elcon_seen_within(ELCON_SILENT_MS) && g_elcon.flags == 0;
}

// ---------------------------------------------------------- bit-rate switch

static bool twai_reinstall(bool charger) {
  twai_stop();
  twai_driver_uninstall();
  twai_general_config_t g = TWAI_GENERAL_CONFIG_DEFAULT(CAN_TX_GPIO, CAN_RX_GPIO, TWAI_MODE_NORMAL);
  g.tx_queue_len = 10;
  twai_timing_config_t t = charger ? (twai_timing_config_t)TWAI_TIMING_CONFIG_250KBITS()
                                   : (twai_timing_config_t)TWAI_TIMING_CONFIG_500KBITS();
  twai_filter_config_t f = TWAI_FILTER_CONFIG_ACCEPT_ALL();
  if (twai_driver_install(&g, &t, &f) != ESP_OK) return false;
  return twai_start() == ESP_OK;
}

bool elcon_bus_enter() { elcon_reset(); return twai_reinstall(true); }
bool elcon_bus_exit()  { return twai_reinstall(false); }