// ti_comm.cpp — builds/parses TI command and response frames.
//
// Command frame:  [INIT][DEV*][REG_H][REG_L][DATA x n][CRC_L][CRC_H]
//                 (* DEV byte only for single-device commands)
// INIT = 1 | REQ_TYPE(3) | rsvd | DATA_SIZE(3)   (DATA_SIZE = n-1, n <= 8)
// For reads, DATA = 1 byte = (number of bytes to read - 1).
//
// Response frame: [INIT = len-1][DEV][REG_H][REG_L][DATA x len][CRC_L][CRC_H]
//
// CRC: CRC-16-IBM, reflected poly 0xA001, init 0xFFFF, appended LSB first.
// Running the CRC over a full frame (including its CRC) gives 0 if valid.
#include "ti_comm.h"
#include "bq_regs.h"

static HardwareSerial& BQ = Serial1;
static uint16_t crc_table[256];

static void build_crc_table() {
  for (int i = 0; i < 256; i++) {
    uint16_t c = (uint16_t)i;
    for (int b = 0; b < 8; b++) c = (c & 1) ? (c >> 1) ^ 0xA001 : (c >> 1);
    crc_table[i] = c;
  }
}

uint16_t ti_crc16(const uint8_t* buf, size_t len) {
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < len; i++) crc = (crc >> 8) ^ crc_table[(crc ^ buf[i]) & 0xFF];
  return crc;
}

void ti_init() {
  build_crc_table();
  BQ.setRxBufferSize(2048);   // stack reads from 12 devices can exceed the 256 B default
  BQ.begin(BQ_UART_BAUD, SERIAL_8N1, PIN_BQ_RX, PIN_BQ_TX);
}

void ti_wake_ping() {
  // Hold the BQ79600 RX line (our TX) low for the wake-ping window.
  BQ.flush();
  BQ.end();
  pinMode(PIN_BQ_TX, OUTPUT);
  digitalWrite(PIN_BQ_TX, LOW);
  delayMicroseconds(WAKE_PING_US);
  digitalWrite(PIN_BQ_TX, HIGH);
  BQ.begin(BQ_UART_BAUD, SERIAL_8N1, PIN_BQ_RX, PIN_BQ_TX);
  delay(WAKE_SETTLE_MS);
}

void ti_flush_rx() {
  while (BQ.available()) BQ.read();
}

static bool send_frame(TiReqType type, uint8_t dev, uint16_t reg,
                       const uint8_t* data, uint8_t len) {
  if (len < 1 || len > 8) return false;
  uint8_t f[16];
  size_t n = 0;
  f[n++] = 0x80 | ((uint8_t)type << 4) | ((len - 1) & 0x07);
  if (type == TI_SINGLE_READ || type == TI_SINGLE_WRITE) f[n++] = dev & 0x3F;
  f[n++] = (uint8_t)(reg >> 8);
  f[n++] = (uint8_t)(reg & 0xFF);
  for (uint8_t i = 0; i < len; i++) f[n++] = data[i];
  uint16_t crc = ti_crc16(f, n);
  f[n++] = (uint8_t)(crc & 0xFF);
  f[n++] = (uint8_t)(crc >> 8);
  return BQ.write(f, n) == n;
}

bool ti_write(TiReqType type, uint8_t dev, uint16_t reg, const uint8_t* data, uint8_t len) {
  return send_frame(type, dev, reg, data, len);
}

bool ti_write1(TiReqType type, uint8_t dev, uint16_t reg, uint8_t value) {
  return send_frame(type, dev, reg, &value, 1);
}

// Reads exactly `n` bytes or times out.
static bool read_exact(uint8_t* buf, size_t n, uint32_t deadline_ms) {
  size_t got = 0;
  while (got < n) {
    if (BQ.available()) buf[got++] = (uint8_t)BQ.read();
    else if ((int32_t)(millis() - deadline_ms) > 0) return false;
  }
  return true;
}

int ti_read(TiReqType type, uint8_t dev, uint16_t reg, uint8_t nbytes,
            int expected_frames, TiResponse* out, uint32_t timeout_ms) {
  for (int i = 0; i < TOTAL_DEVICES; i++) out->valid[i] = false;
  out->num_valid = 0;
  if (nbytes < 1 || nbytes > TI_MAX_RESP_BYTES) return 0;

  ti_flush_rx();
  uint8_t req = nbytes - 1;
  if (!send_frame(type, dev, reg, &req, 1)) return 0;

  uint32_t deadline = millis() + timeout_ms;
  uint8_t frame[TI_MAX_RESP_BYTES + 6];

  for (int k = 0; k < expected_frames; k++) {
    if (!read_exact(frame, 1, deadline)) break;
    if (frame[0] & 0x80) { k--; continue; }          // not a response INIT byte; resync
    size_t dlen = (size_t)(frame[0] & 0x7F) + 1;
    size_t total = dlen + 6;
    if (!read_exact(frame + 1, total - 1, deadline)) break;
    if (ti_crc16(frame, total) != 0) continue;       // bad CRC: drop frame
    uint8_t addr = frame[1] & 0x3F;
    if (addr >= TOTAL_DEVICES || dlen != nbytes) continue;
    memcpy(out->data[addr], frame + 4, dlen);
    if (!out->valid[addr]) { out->valid[addr] = true; out->num_valid++; }
  }
  return out->num_valid;
}