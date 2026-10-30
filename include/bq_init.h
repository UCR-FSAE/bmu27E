// bq_init.h — wake ping, auto-addressing, auto cell balancing.
#pragma once
#include <Arduino.h>
#include "charge_config.h"

// Uses the UART functions in bq79616.cpp. Call bqUartInit() in setup() first.

// Wake the BQ79600, then have it wake the 616 stack.
void bq_wake();

// Auto-address the chain (bridge = 0, 616s = 1..N), verify, clear sync faults.
// Returns false if any device fails to answer with its address.
bool bq_auto_address();

// Wake + auto-address in one call.
bool bq_init();

// Balancing
void bq_config_balancing();   // duty cycle, all cells off
bool bq_start_balancing(const bool want[NUM_STACK_DEVICES][CELLS_PER_DEVICE]);
bool bq_stop_balancing();
bool bq_pause_balancing(bool pause);   // pause while reading cell voltages
bool bq_balancing_active();