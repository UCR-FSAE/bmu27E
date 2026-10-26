// elcon.h — Elcon HK-LF-312-20 CAN interface (250 kbps, 29-bit IDs).
// Works with the team's ESP32_CAN class as-is.
#pragma once
#include <Arduino.h>
#include "ESP32_CAN.h"

struct ElconStatus {
  float    out_v;
  float    out_a;
  uint8_t  flags;        // bit0 HW fail, bit1 over-temp, bit2 input V, bit3 start/battery, bit4 comm timeout
  uint32_t last_seen_ms; // 0 = never
};

extern ElconStatus g_elcon;

void elcon_attach(ESP32_CAN& bus);           // call once after Can.begin()
void elcon_reset();
void elcon_send(float volts, float amps, bool stop);
bool elcon_handle(const CAN_message_t& m);   // true if it was an Elcon status frame
bool elcon_seen_within(uint32_t ms);
bool elcon_healthy();

// ESP32_CAN::begin() is fixed at 500 kbps. These re-initialise the same TWAI
// peripheral at 250 kbps for the Elcon and back to 500 kbps for the car,
// without changing ESP32_CAN.cpp. ESP32_CAN::read()/write() keep working.
bool elcon_bus_enter();   // -> 250 kbps
bool elcon_bus_exit();    // -> 500 kbps