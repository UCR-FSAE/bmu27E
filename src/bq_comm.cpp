/*
    Functions for communication with bq chips

*/

// software design ref (SDR): https://www.ti.com/lit/an/slvae86b/slvae86b.pdf?ts=1775108056517
// datasheet: https://www.ti.com/lit/gpn/bq79616-q1

//hi evan
//hello coner

/*---- command frame ----
{
command type + (num bytes sent - 1), 
device id (only for single device read/write),
register addr 1,
register addr 2,
data
    reading - 1 byte, num bytes requested up to 128 (0x00 = 1 byte, 0x7F = 128 bytes), 
    writing - data to be written [max 8 bytes],
CRC 1,
CRC 2,
}*/
#include <Arduino.h>
#include "bq_comm.h"


// uint8_t message[] = {0x80, 0x02, 0x02, 0x15, 0x0B, 0xCB, 0x49};
// single device read command frame (table 1-2) (SDR p2)

// datasheet p56
uint16_t crc_compute(const uint8_t *data, size_t length) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < length; i++) {
        uint8_t index = (uint8_t)(crc ^ data[i]);
        crc = (crc >> 8) ^ crc_table[index];
    }
    return crc;
}

void bq_wake() {
  // 1) Wake ping to the BQ79600 (TX held low). Twice, as in TI's sample code.
  wakePing();
  wakePing();

  // 2) Bridge sends a wake tone up the daisy chain to the 616s.
  bqWrite1(SINGLE_WRITE_VAL, 0, REG_CONTROL1, CONTROL1_SEND_WAKE);
  // delay(STACK_WAKE_MS_PER_DEV * NUM_STACK_DEVICES);
}

// Message Type Definitions
/*
// Single
#define SINGLE_READ_VAL         0x80    
#define SINGLE_WRITE_VAL        0x90    
// Stack
#define STACK_READ_VAL          0xA0
#define STACK_WRITE_VAL         0xB0
// Broadcast
#define BROAD_READ_VAL          0xC0
#define BROAD_WRITE_VAL         0xD0
#define BROAD_WRITE_REVERSE     0xE0
*/

// Build a complete write frame into buf[], return total frame length
// dev_addr: device address
// msgType:    frame type
// reg_addr: register address (2 bytes for BQ79616)
// data:         payload bytes
// data_len: number of payload bytes (0–8)
uint8_t build_write_frame(uint8_t *buf, uint8_t msgType, uint8_t dev_addr, uint16_t reg_addr, const uint8_t *data, uint8_t data_len) {
    uint8_t i = 0;
    if (data_len < 1 || data_len > 8) return 0;   // ADDED: frame holds 1-8 data bytes; 0 = error
    
    // Header
    buf[i++] = msgType | ((data_len - 1) & 0x07);    // frame type  (FIXED: low 3 bits = data bytes - 1)
    if (msgType == SINGLE_WRITE_VAL) { buf[i++] = dev_addr; }    // device address
    
    buf[i++] = (uint8_t)(reg_addr >> 8);    // register address high byte
    buf[i++] = (uint8_t)(reg_addr & 0xFF); // register address low byte

    // Payload
    for (uint8_t j = 0; j < data_len; j++) {
        buf[i++] = data[j];
    }

    // CRC covers everything above
    uint16_t crc = crc_compute(buf, i);
    buf[i++] = (uint8_t)(crc & 0xFF);    // CRC low byte first
    buf[i++] = (uint8_t)((crc >> 8) & 0xFF); // CRC high byte

    return i; // total frame length
}

// Build a complete read frame into buf[], return total frame length
// dev_addr: device address
// msgType:    frame type
// reg_addr: register address (2 bytes for BQ79616)
// data:     payload bytes
// data_len: number of payload bytes (0–8)
uint8_t build_read_frame(uint8_t *buf, uint8_t msgType, uint8_t dev_addr, uint16_t reg_addr, uint8_t bytes_request) {
    uint8_t i = 0;
    
    // Header
    buf[i++] = msgType;    // frame type
    if (msgType == SINGLE_READ_VAL) { buf[i++] = dev_addr; }    // device address
    
    buf[i++] = (uint8_t)(reg_addr >> 8);    // register address high byte
    buf[i++] = (uint8_t)(reg_addr & 0xFF); // register address low byte

    buf[i++] = bytes_request;

    // CRC covers everything above
    uint16_t crc = crc_compute(buf, i);
    buf[i++] = (uint8_t)(crc & 0xFF);    // CRC low byte first
    buf[i++] = (uint8_t)((crc >> 8) & 0xFF); // CRC high byte

    return i; // total frame length
}

void sendFrame(uint8_t* frame, uint8_t len) {
    UART_SERIAL.write(frame, len);
}

//===================================
// coner's work
//===================================
//placeholders
void readVoltages(){
    //TODO: do it.    
}

void readTemps() {
    //TODO: read like a book.
}

void readFaultFlags() {
    //double check faults that get sent
}

void rebalanceVoltages(){
    //TODO: well figure out how to write first
}

// static BqResponse resp;
// static_assert(TOTAL_DEVICES <= BQ_MAX_DEVICES, "Raise BQ_MAX_DEVICES (here and in bq79616.cpp)");
// static bool balancing = false;

// Read exactly n bytes, or give up at the deadline so a dead board can't hang the code.
static bool readExact(uint8_t *buf, size_t n, uint32_t deadline_ms) {
    size_t got = 0;
    while (got < n) {
        if (UART_SERIAL.available()) buf[got++] = (uint8_t)UART_SERIAL.read();
        else if ((int32_t)(millis() - deadline_ms) > 0) return false;
    }
    return true;
}

// Collect up to `expected_frames` replies of `nbytes` data bytes each.
// Reply frame: [len-1][dev][reg_H][reg_L][data x len][CRC_L][CRC_H]
// Returns how many valid (CRC-checked) replies arrived.
int bqReadResponse(uint8_t nbytes, int expected_frames, BqResponse *out, uint32_t timeout_ms) {
    for (int i = 0; i < BQ_MAX_DEVICES; i++) out->valid[i] = false;
    out->num_valid = 0;

    uint32_t deadline = millis() + timeout_ms;
    uint8_t frame[BQ_MAX_RESP_BYTES + 6];

    for (int k = 0; k < expected_frames; k++) {
        if (!readExact(frame, 1, deadline)) break;
        if (frame[0] & 0x80) { k--; continue; }            // not a reply start byte, resync
        size_t dlen  = (size_t)(frame[0] & 0x7F) + 1;
        size_t total = dlen + 6;
        if (!readExact(frame + 1, total - 1, deadline)) break;
        if (crc_compute(frame, total) != 0) continue;      // CRC over whole frame = 0 if intact
        uint8_t addr = frame[1] & 0x3F;
        if (addr >= BQ_MAX_DEVICES || dlen != nbytes) continue;
        memcpy(out->data[addr], frame + 4, dlen);
        if (!out->valid[addr]) { out->valid[addr] = true; out->num_valid++; }
    }
    return out->num_valid;
}


// Write 1-8 bytes. msgType = SINGLE_WRITE_VAL / STACK_WRITE_VAL / BROAD_WRITE_VAL.
bool bqWrite(uint8_t msgType, uint8_t dev_addr, uint16_t reg_addr, const uint8_t *data, uint8_t data_len) {
    uint8_t buf[16];
    uint8_t len = build_write_frame(buf, msgType, dev_addr, reg_addr, data, data_len);
    if (len == 0) return false;
    sendFrame(buf, len);
    return true;
}

bool bqWrite1(uint8_t msgType, uint8_t dev_addr, uint16_t reg_addr, uint8_t value) {
    return bqWrite(msgType, dev_addr, reg_addr, &value, 1);
}

// Read `nbytes` (real count, 1-128) and collect the replies.
// msgType = SINGLE_READ_VAL / STACK_READ_VAL / BROAD_READ_VAL.
int bqRead(uint8_t msgType, uint8_t dev_addr, uint16_t reg_addr, uint8_t nbytes,
           int expected_frames, BqResponse *out, uint32_t timeout_ms) {
    if (nbytes < 1 || nbytes > BQ_MAX_RESP_BYTES) return 0;
    uint8_t buf[16];
    bqFlushRx();
    uint8_t len = build_read_frame(buf, msgType, dev_addr, reg_addr, nbytes - 1);  // frame wants bytes - 1
    sendFrame(buf, len);
    return bqReadResponse(nbytes, expected_frames, out, timeout_ms);
}