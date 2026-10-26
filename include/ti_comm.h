// ti_comm.h — TI daisy-chain frame layer (BQ79600 over UART).
#pragma once
#include <Arduino.h>
#include "charge_config.h"

enum TiReqType : uint8_t {
  TI_SINGLE_READ   = 0,
  TI_SINGLE_WRITE  = 1,
  TI_STACK_READ    = 2,
  TI_STACK_WRITE   = 3,
  TI_BCAST_READ    = 4,
  TI_BCAST_WRITE   = 5,
  TI_BCAST_WRITE_REV = 6,
};

#define TI_MAX_RESP_BYTES 128

// Responses sorted by device address (0 = bridge, 1..N = stack).
struct TiResponse {
  uint8_t data[TOTAL_DEVICES][TI_MAX_RESP_BYTES];
  bool    valid[TOTAL_DEVICES];
  int     num_valid;
};

void     ti_init();
void     ti_wake_ping();
uint16_t ti_crc16(const uint8_t* buf, size_t len);

// Writes: len = 1..8 data bytes. `dev` is ignored for stack/broadcast.
bool ti_write(TiReqType type, uint8_t dev, uint16_t reg, const uint8_t* data, uint8_t len);
bool ti_write1(TiReqType type, uint8_t dev, uint16_t reg, uint8_t value);

// Reads: sends the command and collects `expected_frames` responses of `nbytes` each.
// Returns number of valid (CRC-OK) frames.
int  ti_read(TiReqType type, uint8_t dev, uint16_t reg, uint8_t nbytes,
             int expected_frames, TiResponse* out, uint32_t timeout_ms = 10);

void ti_flush_rx();