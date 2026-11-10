/*
    Functions for communication with bq chips
*/

// software design ref (SDR): https://www.ti.com/lit/an/slvae86b/slvae86b.pdf?ts=1775108056517
// datasheet: https://www.ti.com/lit/gpn/bq79616-q1

//hi evan
//hello coner

#include <Arduino.h>
#include "bq_comm.h"

// ---------------------------------------------------------------------------
//                      msg builder helper functions
// ---------------------------------------------------------------------------
// datasheet p56
uint16_t crc_compute(const uint8_t *data, size_t length) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < length; i++) {
        uint8_t index = (uint8_t)(crc ^ data[i]);
        crc = (crc >> 8) ^ crc_table[index];
    }
    return crc;
}

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

// Not needed, here just in case we need in the future but for now ignore
// void sendFrame(uint8_t* frame, uint8_t len) {
//     UART_SERIAL.write(frame, len);
// }

// Write 1-8 bytes. msgType = SINGLE_WRITE_VAL / STACK_WRITE_VAL / BROAD_WRITE_VAL.
bool bqWrite(uint8_t msgType, uint8_t dev_addr, uint16_t reg_addr, const uint8_t *data, uint8_t data_len) {
    uint8_t buf[16];
    uint8_t len = build_write_frame(buf, msgType, dev_addr, reg_addr, data, data_len);
    if (len == 0) return false;
    UART_SERIAL.write(buf, len);
    return true;
}

bool bqWrite1(uint8_t msgType, uint8_t dev_addr, uint16_t reg_addr, uint8_t value) {
    return bqWrite(msgType, dev_addr, reg_addr, &value, 1);
}

// send read frame and read from uart
BmsStatus_t bqRead(uint8_t msgType, uint8_t dev_addr, uint16_t reg_addr, uint8_t nbytes,
                   int expected_frames, BqResponse *out, uint32_t timeout_ms) {
    BmsStatus_t st = BMS_ERR_TIMEOUT;
    for (uint8_t attempt = 0; attempt <= BQ_MAX_RETRIES; attempt++) {
        if (nbytes < 1 || nbytes > BQ_MAX_RESP_BYTES) return BMS_ERR_PARAM;   // see note below
        uint8_t buf[16];
        bqFlushRx();                                   // also discards leftovers from a failed attempt
        uint8_t len = build_read_frame(buf, msgType, dev_addr, reg_addr, nbytes - 1);
        UART_SERIAL.write(buf, len);
        st = bqReadResponse(nbytes, expected_frames, out, timeout_ms);
        if (st != BMS_ERR_CRC) return st;          // OK or a non-retryable error
    }
    return st;                                     // still BMS_ERR_CRC after all retries
}

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
BmsStatus_t bqReadResponse(uint8_t nbytes, int expected_frames, BqResponse *out, uint32_t timeout_ms) {
    for (int i = 0; i < BQ_MAX_DEVICES; i++) out->valid[i] = false;
    out->num_valid  = 0;
    out->crc_errors = 0; 
    
    uint32_t deadline = millis() + timeout_ms;
    uint8_t frame[BQ_MAX_RESP_BYTES + 6];
    
    for (int k = 0; k < expected_frames; ) {
        if (!readExact(frame, 1, deadline)) { break; }
        if (frame[0] & 0x80) { continue; }                     // not a reply start byte, resync
        size_t dlen  = (size_t)(frame[0] & 0x7F) + 1;
        if (dlen > BQ_MAX_RESP_BYTES) continue;            // would overflow frame[], resync
        size_t total = dlen + 6;
        if (!readExact(frame + 1, total - 1, deadline)) { break; }
        if (crc_compute(frame, total) != 0) { out->crc_errors++; continue; }     // bad CRC, don't count it
        uint8_t addr = frame[1] & 0x3F;
        if (addr >= BQ_MAX_DEVICES || dlen != nbytes) { continue; }
        memcpy(out->data[addr], frame + 4, dlen);
        if (!out->valid[addr]) { out->valid[addr] = true; out->num_valid++; }
        k++;                                               // only count good frames
    }

    if (out->num_valid >= expected_frames) return BMS_OK;
    return out->crc_errors ? BMS_ERR_CRC : BMS_ERR_TIMEOUT;
}

// ---------------------------------------------------------------------------
//                              init functions
// ---------------------------------------------------------------------------

void wakePing() {
    UART_SERIAL.flush();    
    UART_SERIAL.end();         
    pinMode(TX_PIN, OUTPUT);
    digitalWrite(TX_PIN, HIGH);
    digitalWrite(TX_PIN, LOW);
    delayMicroseconds(WAKE_PING_US);
    digitalWrite(TX_PIN, HIGH);
    UART_SERIAL.begin(BQ_UART_BAUD, SERIAL_8N1, RX_PIN, TX_PIN);
    delay(4);
}

// Call once in setup(). Bigger RX buffer because a stack read from 12 boards
// returns more than the 256-byte default.
void bqUartInit() {
    UART_SERIAL.setRxBufferSize(2048);
    UART_SERIAL.begin(BQ_UART_BAUD, SERIAL_8N1, RX_PIN, TX_PIN);
}

// Throw away any old bytes so they aren't mistaken for a new reply.
void bqFlushRx() {
    while (UART_SERIAL.available()) UART_SERIAL.read();
}

void bq_wake() {
    wakePing();                        // RX low for WAKE_PING_US (tHLD_WAKE 2.5-3 ms)
    delay(WAKE_SETTLE_MS);             // > tSU(WAKE_SHUT) max 3.5 ms
    wakePing();                        // second ping
    delay(1);                          // > tRST 600 us before first frame
    
    bqWrite1(SINGLE_WRITE_VAL, 0, REG_CONTROL1, CONTROL1_SEND_WAKE);
    
    delay(STACK_WAKE_MS);              // wake tone ripples through all stack devices
}

bool bq_auto_address() {
    static_assert(TOTAL_DEVICES == NUM_STACK_DEVICES + 1, "TOTAL_DEVICES must be bridge + stack devices");

    // Step 1: dummy STACK write 0x00 to 0x343..0x34A, 8 separate 1-byte writes (DLL sync)
    for (uint16_t i = 0; i < 8; i++) {
        bqWrite1(STACK_WRITE_VAL, 0, REG_OTP_ECC_DATAIN1 + i, 0x00);
    }

    // Step 2: enable auto-addressing
    bqWrite1(BROAD_WRITE_VAL, 0, REG_CONTROL1, CONTROL1_ADDR_WR);

    // Step 3: consecutive addresses, bridge = 0, 616s = 1..12
    for (uint8_t a = 0; a < TOTAL_DEVICES; a++) {
        bqWrite1(BROAD_WRITE_VAL, 0, REG_DIR0_ADDR, a);
    }

    // Step 4: stack devices, then top of stack (bridge is base by default, has no COMM_CTRL)
    bqWrite1(BROAD_WRITE_VAL, 0, REG_COMM_CTRL, 0x02);
    bqWrite1(SINGLE_WRITE_VAL, NUM_STACK_DEVICES, REG_COMM_CTRL, 0x03);

    // Step 5: dummy STACK read of 0x343..0x34A, one command per register (DLL sync).
    // Result ignored on purpose; some responses may not arrive.
    for (uint16_t i = 0; i < 8; i++) {
        (void)bqRead(STACK_READ_VAL, 0, REG_OTP_ECC_DATAIN1 + i, 1, NUM_STACK_DEVICES, &resp, 20);
    }

    // Step 7: verify addresses of all 12 stack devices
    BmsStatus_t st = bqRead(STACK_READ_VAL, 0, REG_DIR0_ADDR, 1, NUM_STACK_DEVICES, &resp, 20);
    bool ok = (st == BMS_OK);
    if (st != BMS_OK) {
        Serial.printf("[BQ] stack addr read failed: status %d (%d/%d replies, %d CRC errors)\n",
                      (int)st, resp.num_valid, NUM_STACK_DEVICES, resp.crc_errors);
    }
    for (int a = 1; a <= NUM_STACK_DEVICES; a++) {
        bool good = resp.valid[a] && (resp.data[a][0] & 0x3F) == a;
        Serial.printf("[BQ] device %d: %s\n", a, good ? "OK" : "MISSING/WRONG");
        ok &= good;
    }

    // Bridge: not part of a stack read. Datasheet check is DEV_CONF1 (0x2001) == 0x14.
    BmsStatus_t stb = bqRead(SINGLE_READ_VAL, 0, REG_DEV_CONF1_600, 1, 1, &resp, 20);
    bool bridgeGood = (stb == BMS_OK) && resp.valid[0] && resp.data[0][0] == 0x14;
    if (stb != BMS_OK) {
        Serial.printf("[BQ] bridge read failed: status %d\n", (int)stb);
    }
    Serial.printf("[BQ] device 0 (bridge): %s\n", bridgeGood ? "OK" : "MISSING/WRONG");
    ok &= bridgeGood;

    // Step 8: clear faults latched by the DLL sync
    uint8_t ff[2] = {0xFF, 0x7F};                             // bit 7 of FAULT_RST2 is reserved
    bqWrite(STACK_WRITE_VAL, 0, REG_FAULT_RST1, ff, 2);       // 0x331 + 0x332
    bqWrite1(SINGLE_WRITE_VAL, 0, REG_FAULT_RST_600, 0xFF);   // bridge 0x2030

    return ok;
}

// ---------------------------------------------------------------------------
//                          Cell Data reading
// ---------------------------------------------------------------------------

void readVoltages(){
    //TODO: do it.    
}

void readTemps() {
    //TODO: read like a book.
}

void readFaultFlags() {
    //double check faults that get sent
}