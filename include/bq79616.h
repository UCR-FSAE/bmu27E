#ifndef BQ79616_H
#define BQ79616_H

// BQ79600 / BQ79616 UART driver
// Software design ref (SDR): https://www.ti.com/lit/an/slvae86b/slvae86b.pdf
// Datasheet:                 https://www.ti.com/lit/gpn/bq79616-q1

#include <Arduino.h>
#include <stddef.h>
#include <stdint.h>

// ---------------------------------------------------------------------------
// Configuration
// ---------------------------------------------------------------------------
#define BQ_UART_BAUD       1000000   // BQ79600 UART speed
#define WAKE_PING_US       2750      // wake ping low time (check tHLD_WAKE in BQ79600 datasheet)
#define BQ_MAX_DEVICES     13        // BQ79600 (addr 0) + 12 x BQ79616 (addr 1..12)
#define BQ_MAX_RESP_BYTES  128       // largest read the chips allow

// ---------------------------------------------------------------------------
// Message type definitions (command byte upper nibble)
// ---------------------------------------------------------------------------
// Single device
#define SINGLE_READ_VAL     0x80
#define SINGLE_WRITE_VAL    0x90
// Stack
#define STACK_READ_VAL      0xA0
#define STACK_WRITE_VAL     0xB0
// Broadcast
#define BROAD_READ_VAL      0xC0
#define BROAD_WRITE_VAL     0xD0
#define BROAD_WRITE_REVERSE 0xE0

// ---------------------------------------------------------------------------
// Types
// ---------------------------------------------------------------------------
// Replies stored by the device address inside each reply
// (0 = BQ79600, 1..N = BQ79616s).
struct BqResponse {
    uint8_t data[BQ_MAX_DEVICES][BQ_MAX_RESP_BYTES];
    bool    valid[BQ_MAX_DEVICES];
    int     num_valid;
};

// ---------------------------------------------------------------------------
// CRC
// ---------------------------------------------------------------------------
// CRC-16 (datasheet p56). Running it over a complete, intact frame returns 0.
uint16_t crc_compute(const uint8_t *data, size_t length);

// ---------------------------------------------------------------------------
// Frame builders (return total frame length, 0 on error)
// ---------------------------------------------------------------------------
// data_len: 1-8 payload bytes. dev_addr only used for SINGLE_WRITE_VAL.
uint8_t build_write_frame(uint8_t *buf, uint8_t msgType, uint8_t dev_addr,
                          uint16_t reg_addr, const uint8_t *data, uint8_t data_len);

// bytes_request: real byte count minus 1 (0x00 = 1 byte, 0x7F = 128 bytes).
// dev_addr only used for SINGLE_READ_VAL.
uint8_t build_read_frame(uint8_t *buf, uint8_t msgType, uint8_t dev_addr,
                         uint16_t reg_addr, uint8_t bytes_request);

// ---------------------------------------------------------------------------
// Low-level UART
// ---------------------------------------------------------------------------
void sendFrame(uint8_t *frame, uint8_t len);
void wakePing();

// Call once in setup(). Enlarges the RX buffer for stack reads.
void bqUartInit();

// Discard any stale bytes before a new read.
void bqFlushRx();

// ---------------------------------------------------------------------------
// High-level read / write
// ---------------------------------------------------------------------------
// Collect up to expected_frames replies of nbytes data bytes each.
// Returns the number of valid (CRC-checked) replies.
int bqReadResponse(uint8_t nbytes, int expected_frames, BqResponse *out, uint32_t timeout_ms);

// Write 1-8 bytes. msgType = SINGLE_/STACK_/BROAD_WRITE_VAL.
bool bqWrite(uint8_t msgType, uint8_t dev_addr, uint16_t reg_addr,
             const uint8_t *data, uint8_t data_len);
bool bqWrite1(uint8_t msgType, uint8_t dev_addr, uint16_t reg_addr, uint8_t value);

// Read nbytes (real count, 1-128) and collect replies.
// msgType = SINGLE_/STACK_/BROAD_READ_VAL.
int bqRead(uint8_t msgType, uint8_t dev_addr, uint16_t reg_addr, uint8_t nbytes,
           int expected_frames, BqResponse *out, uint32_t timeout_ms);

// ---------------------------------------------------------------------------
// Application-level (placeholders, still TODO)
// ---------------------------------------------------------------------------
void readVoltages();
void readTemps();
void readFaultFlags();
void rebalanceVoltages();

// ---------------------------------------------------------------------------
// Test helpers
// ---------------------------------------------------------------------------
void testTeensyWrite();
void testBQWrite();

#endif // BQ79616_H
