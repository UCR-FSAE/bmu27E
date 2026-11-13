/*
    Functions for communication with bq chips
*/

// software design ref (SDR): https://www.ti.com/lit/an/slvae86b/slvae86b.pdf?ts=1775108056517
// datasheet: https://www.ti.com/lit/gpn/bq79616-q1

// hi evan
// hello coner

#include <Arduino.h>
#include "bq_comm.h"

// global response object
BqResponse resp;
uint32_t last_resp[READ_COUNT];
bool stack_addressed = false;

// ---------------------------------------------------------------------------
//                      msg builder helper functions
// ---------------------------------------------------------------------------

// crc lookup table
const uint16_t crc_table[256] = {
    0x0000,
    0xC0C1,
    0xC181,
    0x0140,
    0xC301,
    0x03C0,
    0x0280,
    0xC241,
    0xC601,
    0x06C0,
    0x0780,
    0xC741,
    0x0500,
    0xC5C1,
    0xC481,
    0x0440,
    0xCC01,
    0x0CC0,
    0x0D80,
    0xCD41,
    0x0F00,
    0xCFC1,
    0xCE81,
    0x0E40,
    0x0A00,
    0xCAC1,
    0xCB81,
    0x0B40,
    0xC901,
    0x09C0,
    0x0880,
    0xC841,
    0xD801,
    0x18C0,
    0x1980,
    0xD941,
    0x1B00,
    0xDBC1,
    0xDA81,
    0x1A40,
    0x1E00,
    0xDEC1,
    0xDF81,
    0x1F40,
    0xDD01,
    0x1DC0,
    0x1C80,
    0xDC41,
    0x1400,
    0xD4C1,
    0xD581,
    0x1540,
    0xD701,
    0x17C0,
    0x1680,
    0xD641,
    0xD201,
    0x12C0,
    0x1380,
    0xD341,
    0x1100,
    0xD1C1,
    0xD081,
    0x1040,
    0xF001,
    0x30C0,
    0x3180,
    0xF141,
    0x3300,
    0xF3C1,
    0xF281,
    0x3240,
    0x3600,
    0xF6C1,
    0xF781,
    0x3740,
    0xF501,
    0x35C0,
    0x3480,
    0xF441,
    0x3C00,
    0xFCC1,
    0xFD81,
    0x3D40,
    0xFF01,
    0x3FC0,
    0x3E80,
    0xFE41,
    0xFA01,
    0x3AC0,
    0x3B80,
    0xFB41,
    0x3900,
    0xF9C1,
    0xF881,
    0x3840,
    0x2800,
    0xE8C1,
    0xE981,
    0x2940,
    0xEB01,
    0x2BC0,
    0x2A80,
    0xEA41,
    0xEE01,
    0x2EC0,
    0x2F80,
    0xEF41,
    0x2D00,
    0xEDC1,
    0xEC81,
    0x2C40,
    0xE401,
    0x24C0,
    0x2580,
    0xE541,
    0x2700,
    0xE7C1,
    0xE681,
    0x2640,
    0x2200,
    0xE2C1,
    0xE381,
    0x2340,
    0xE101,
    0x21C0,
    0x2080,
    0xE041,
    0xA001,
    0x60C0,
    0x6180,
    0xA141,
    0x6300,
    0xA3C1,
    0xA281,
    0x6240,
    0x6600,
    0xA6C1,
    0xA781,
    0x6740,
    0xA501,
    0x65C0,
    0x6480,
    0xA441,
    0x6C00,
    0xACC1,
    0xAD81,
    0x6D40,
    0xAF01,
    0x6FC0,
    0x6E80,
    0xAE41,
    0xAA01,
    0x6AC0,
    0x6B80,
    0xAB41,
    0x6900,
    0xA9C1,
    0xA881,
    0x6840,
    0x7800,
    0xB8C1,
    0xB981,
    0x7940,
    0xBB01,
    0x7BC0,
    0x7A80,
    0xBA41,
    0xBE01,
    0x7EC0,
    0x7F80,
    0xBF41,
    0x7D00,
    0xBDC1,
    0xBC81,
    0x7C40,
    0xB401,
    0x74C0,
    0x7580,
    0xB541,
    0x7700,
    0xB7C1,
    0xB681,
    0x7640,
    0x7200,
    0xB2C1,
    0xB381,
    0x7340,
    0xB101,
    0x71C0,
    0x7080,
    0xB041,
    0x5000,
    0x90C1,
    0x9181,
    0x5140,
    0x9301,
    0x53C0,
    0x5280,
    0x9241,
    0x9601,
    0x56C0,
    0x5780,
    0x9741,
    0x5500,
    0x95C1,
    0x9481,
    0x5440,
    0x9C01,
    0x5CC0,
    0x5D80,
    0x9D41,
    0x5F00,
    0x9FC1,
    0x9E81,
    0x5E40,
    0x5A00,
    0x9AC1,
    0x9B81,
    0x5B40,
    0x9901,
    0x59C0,
    0x5880,
    0x9841,
    0x8801,
    0x48C0,
    0x4980,
    0x8941,
    0x4B00,
    0x8BC1,
    0x8A81,
    0x4A40,
    0x4E00,
    0x8EC1,
    0x8F81,
    0x4F40,
    0x8D01,
    0x4DC0,
    0x4C80,
    0x8C41,
    0x4400,
    0x84C1,
    0x8581,
    0x4540,
    0x8701,
    0x47C0,
    0x4680,
    0x8641,
    0x8201,
    0x42C0,
    0x4380,
    0x8341,
    0x4100,
    0x81C1,
    0x8081,
    0x4040,
};

// datasheet p56
uint16_t crc_compute(const uint8_t *data, size_t length)
{
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < length; i++)
    {
        uint8_t index = (uint8_t)(crc ^ data[i]);
        crc = (crc >> 8) ^ crc_table[index];
    }
    return crc;
}

// Build a complete write frame into buf[], return total frame length
// dev_addr: device ad
// msgType:    frame type
// reg_addr: register ad (2 bytes for BQ79616)
// data:         payload bytes
// data_len: number of payload bytes (0–8)
uint8_t build_write_frame(uint8_t *buf, uint8_t msgType, uint8_t dev_addr, uint16_t reg_addr, const uint8_t *data, uint8_t data_len)
{
    uint8_t i = 0;
    if (data_len < 1 || data_len > 8)
        return 0; // ADDED: frame holds 1-8 data bytes; 0 = error

    // Header
    buf[i++] = msgType | ((data_len - 1) & 0x07); // frame type  (FIXED: low 3 bits = data bytes - 1)
    if (msgType == SINGLE_WRITE_VAL)
    {
        buf[i++] = dev_addr;
    } // device ad

    buf[i++] = (uint8_t)(reg_addr >> 8);   // register ad high byte
    buf[i++] = (uint8_t)(reg_addr & 0xFF); // register ad low byte

    // Payload
    for (uint8_t j = 0; j < data_len; j++)
    {
        buf[i++] = data[j];
    }

    // CRC covers everything above
    uint16_t crc = crc_compute(buf, i);
    buf[i++] = (uint8_t)(crc & 0xFF);        // CRC low byte first
    buf[i++] = (uint8_t)((crc >> 8) & 0xFF); // CRC high byte

    return i; // total frame length
}

// Build a complete read frame into buf[], return total frame length
// dev_addr: device ad
// msgType:    frame type
// reg_addr: register ad (2 bytes for BQ79616)
// data:     payload bytes
// data_len: number of payload bytes (0–8)
uint8_t build_read_frame(uint8_t *buf, uint8_t msgType, uint8_t dev_addr, uint16_t reg_addr, uint8_t bytes_request)
{
    uint8_t i = 0;

    // Header
    buf[i++] = msgType; // frame type
    if (msgType == SINGLE_READ_VAL)
    {
        buf[i++] = dev_addr;
    } // device ad

    buf[i++] = (uint8_t)(reg_addr >> 8);   // register ad high byte
    buf[i++] = (uint8_t)(reg_addr & 0xFF); // register ad low byte

    buf[i++] = bytes_request;

    // CRC covers everything above
    uint16_t crc = crc_compute(buf, i);
    buf[i++] = (uint8_t)(crc & 0xFF);        // CRC low byte first
    buf[i++] = (uint8_t)((crc >> 8) & 0xFF); // CRC high byte

    return i; // total frame length
}

// Not needed, here just in case we need in the future but for now ignore
// void sendFrame(uint8_t* frame, uint8_t len) {
//     UART_SERIAL.write(frame, len);
// }

// Write 1-8 bytes. msgType = SINGLE_WRITE_VAL / STACK_WRITE_VAL / BROAD_WRITE_VAL.
bool bqWrite(uint8_t msgType, uint8_t dev_addr, uint16_t reg_addr, const uint8_t *data, uint8_t data_len)
{
    uint8_t buf[16];
    uint8_t len = build_write_frame(buf, msgType, dev_addr, reg_addr, data, data_len);
    if (len == 0)
        return false;
    UART_SERIAL.write(buf, len);
    return true;
}

bool bqWrite1(uint8_t msgType, uint8_t dev_addr, uint16_t reg_addr, uint8_t value)
{
    return bqWrite(msgType, dev_addr, reg_addr, &value, 1);
}

// send read frame and read from uart
BmsStatus_t bqRead(uint8_t msgType, uint8_t dev_addr, uint16_t reg_addr, uint8_t nbytes,
                   int expected_frames, Bqresp *out, uint32_t timeout_ms)
{
    BmsStatus_t st = BMS_ERR_TIMEOUT;
    for (uint8_t attempt = 0; attempt <= BQ_MAX_RETRIES; attempt++)
    {
        if (nbytes < 1 || nbytes > BQ_MAX_RESP_BYTES)
            return BMS_ERR_PARAM; // see note below
        uint8_t buf[16];
        bqFlushRx(); // also discards leftovers from a failed attempt
        uint8_t len = build_read_frame(buf, msgType, dev_addr, reg_addr, nbytes - 1);
        UART_SERIAL.write(buf, len);
        st = bqReadresp(nbytes, expected_frames, out, timeout_ms);
        if (st != BMS_ERR_CRC)
            return st; // OK or a non-retryable error
    }
    return st; // still BMS_ERR_CRC after all retries
}

// Read exactly n bytes, or give up at the deadline so a dead board can't hang the code.
static bool readExact(uint8_t *buf, size_t n, uint32_t deadline_ms)
{
    size_t got = 0;
    while (got < n)
    {
        if (UART_SERIAL.available())
            buf[got++] = (uint8_t)UART_SERIAL.read();
        else if ((int32_t)(millis() - deadline_ms) > 0)
            return false;
    }
    return true;
}

// Collect up to `expected_frames` replies of `nbytes` data bytes each.
// Reply frame: [len-1][dev][reg_H][reg_L][data x len][CRC_L][CRC_H]
// Returns how many valid (CRC-checked) replies arrived.
BmsStatus_t bqReadresp(uint8_t nbytes, int expected_frames, Bqresp *out, uint32_t timeout_ms)
{
    for (int i = 0; i < BQ_MAX_DEVICES; i++)
        out->valid[i] = false;
    out->num_valid = 0;
    out->crc_errors = 0;

    uint32_t deadline = millis() + timeout_ms;
    uint8_t frame[BQ_MAX_RESP_BYTES + 6];

    for (int k = 0; k < expected_frames;)
    {
        if (!readExact(frame, 1, deadline))
        {
            break;
        }
        if (frame[0] & 0x80)
        {
            continue;
        } // not a reply start byte, resync
        size_t dlen = (size_t)(frame[0] & 0x7F) + 1;
        if (dlen > BQ_MAX_RESP_BYTES)
            continue; // would overflow frame[], resync
        size_t total = dlen + 6;
        if (!readExact(frame + 1, total - 1, deadline))
        {
            break;
        }
        if (crc_compute(frame, total) != 0)
        {
            out->crc_errors++;
            continue;
        } // bad CRC, don't count it
        uint8_t addr = frame[1] & 0x3F;
        if (addr >= BQ_MAX_DEVICES || dlen != nbytes)
        {
            continue;
        }
        memcpy(out->data[addr], frame + 4, dlen);
        if (!out->valid[addr])
        {
            out->valid[addr] = true;
            out->num_valid++;
        }
        k++; // only count good frames
    }

    if (out->num_valid >= expected_frames)
        return BMS_OK;
    return out->crc_errors ? BMS_ERR_CRC : BMS_ERR_TIMEOUT;
}

// ---------------------------------------------------------------------------
//                              init functions
// ---------------------------------------------------------------------------

void wakePing()
{
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
void bqUartInit()
{
    UART_SERIAL.setRxBufferSize(2048);
    UART_SERIAL.begin(BQ_UART_BAUD, SERIAL_8N1, RX_PIN, TX_PIN);
}

// Throw away any old bytes so they aren't mistaken for a new reply.
void bqFlushRx()
{
    while (UART_SERIAL.available())
        UART_SERIAL.read();
}

void bq_wake()
{
    wakePing();            // RX low for WAKE_PING_US (tHLD_WAKE 2.5-3 ms)
    delay(WAKE_SETTLE_MS); // > tSU(WAKE_SHUT) max 3.5 ms
    wakePing();            // second ping
    delay(1);              // > tRST 600 us before first frame

    bqWrite1(SINGLE_WRITE_VAL, 0, REG_CONTROL1, CONTROL1_SEND_WAKE);

    delay(STACK_WAKE_MS); // wake tone ripples through all stack devices
}

bool bq_auto_address()
{
    static_assert(TOTAL_DEVICES == NUM_STACK_DEVICES + 1, "TOTAL_DEVICES must be bridge + stack devices");
    stack_addressed = false;
    // Step 1: dummy STACK write 0x00 to 0x343..0x34A, 8 separate 1-byte writes (DLL sync)
    for (uint16_t i = 0; i < 8; i++)
    {
        bqWrite1(STACK_WRITE_VAL, 0, REG_OTP_ECC_DATAIN1 + i, 0x00);
    }

    // Step 2: enable auto-addressing
    bqWrite1(BROAD_WRITE_VAL, 0, REG_CONTROL1, CONTROL1_ADDR_WR);

    // Step 3: consecutive addresses, bridge = 0, 616s = 1..12
    for (uint8_t a = 0; a < TOTAL_DEVICES; a++)
    {
        bqWrite1(BROAD_WRITE_VAL, 0, REG_DIR0_ADDR, a);
    }

    // Step 4: stack devices, then top of stack (bridge is base by default, has no COMM_CTRL)
    bqWrite1(BROAD_WRITE_VAL, 0, REG_COMM_CTRL, 0x02);
    bqWrite1(SINGLE_WRITE_VAL, NUM_STACK_DEVICES, REG_COMM_CTRL, 0x03);

    // Step 5: dummy STACK read of 0x343..0x34A, one command per register (DLL sync).
    // Result ignored on purpose; some responses may not arrive.
    for (uint16_t i = 0; i < 8; i++)
    {
        bqRead(STACK_READ_VAL, 0, REG_OTP_ECC_DATAIN1 + i, 1, NUM_STACK_DEVICES, &resp, 20);
    }

    // Step 7: verify addresses of all 12 stack devices
    BmsStatus_t st = bqRead(STACK_READ_VAL, 0, REG_DIR0_ADDR, 1, NUM_STACK_DEVICES, &resp, 20);
    bool ok = (st == BMS_OK);
    if (st != BMS_OK)
    {
        Serial.printf("[BQ] stack addr read failed: status %d (%d/%d replies, %d CRC errors)\n",
                      (int)st, resp.num_valid, NUM_STACK_DEVICES, resp.crc_errors);
    }
    for (int a = 1; a <= NUM_STACK_DEVICES; a++)
    {
        bool good = resp.valid[a] && (resp.data[a][0] & 0x3F) == a;
        Serial.printf("[BQ] device %d: %s\n", a, good ? "OK" : "MISSING/WRONG");
        ok &= good;
    }

    // Bridge: not part of a stack read. Datasheet check is DEV_CONF1 (0x2001) == 0x14.
    BmsStatus_t stb = bqRead(SINGLE_READ_VAL, 0, REG_DEV_CONF1_600, 1, 1, &resp, 20);
    bool bridgeGood = (stb == BMS_OK) && resp.valid[0] && resp.data[0][0] == 0x14;
    if (stb != BMS_OK)
    {
        Serial.printf("[BQ] bridge read failed: status %d\n", (int)stb);
    }
    Serial.printf("[BQ] device 0 (bridge): %s\n", bridgeGood ? "OK" : "MISSING/WRONG");
    ok &= bridgeGood;

    // Step 8: clear faults latched by the DLL sync
    uint8_t ff[2] = {0xFF, 0x7F};                           // bit 7 of FAULT_RST2 is reserved
    bqWrite(STACK_WRITE_VAL, 0, REG_FAULT_RST1, ff, 2);     // 0x331 + 0x332
    bqWrite1(SINGLE_WRITE_VAL, 0, REG_FAULT_RST_600, 0xFF); // bridge 0x2030

    if (ok)
    {
        resetAll();
    }
    return ok;
}

// ---------------------------------------------------------------------------
//                          MSG timout functions
// ---------------------------------------------------------------------------

// call when bqRead == BMS_OK
void markOk(Meas m) { last_resp[m] = millis(); }

// call at end of INIT
void resetAll()
{
    uint32_t now = millis();
    for (int i = 0; i < READ_COUNT; i++)
    {
        last_resp[i] = millis();
    }
    stack_addressed = true;
}

// shut down if msg timed out
bool commLost()
{
    if (!stack_addressed)
    {
        return true;
    }
    uint32_t now = millis();
    for (int i = 0; i < READ_COUNT; i++)
        if (now - last_resp[i] > COMM_TIMEOUT_MS)
        {
            return true;
        }
    return false;
}

// ---------------------------------------------------------------------------
//                          Cell reading functions
// ---------------------------------------------------------------------------

void readVoltages()
{
    // TODO: do it.
}

void readTemps()
{
    // TODO: read like a book.
}

void readFaultFlags()
{
    // double check faults that get sent
}

FaultSummary decodeFaultSummary(uint8_t summary)
{
    FaultSummary f{};
    f.protection = (summary & (1u << 7)) != 0;
    f.comparatorADC = (summary & (1u << 6)) != 0;
    f.otp = (summary & (1u << 5)) != 0;
    f.communication = (summary & (1u << 4)) != 0;
    f.overTempUnderTemp = (summary & (1u << 3)) != 0;
    f.overVoltageUnderVoltage = (summary & (1u << 2)) != 0;
    f.system = (summary & (1u << 1)) != 0;
    f.power = (summary & 1u) != 0;
    return f;
}

BmsStatus_t readFaultSummary(Bqresp out)
{
    BmsStatus_t status = bqRead(STACK_READ_VAL, // reading from the stack to get all 12 resps at once
                                0,
                                REG_FAULT_SMRY,
                                1,
                                12,
                                &out,
                                20);

    if (status != BMS_OK) // do smth

        // check if each respose is valid
        for (int i = 1; i <= 12; ++i)
        {
            if (!out.valid[i])
            {
                // do smth
            }
        }
    return status;
}

// TODO
// should I make the arrays 12 sized and shift everything left?
BmsStatus_t handleFaultSummary(FaultSummary summary, FaultRegisters registers)
{
    // substitute resp with global resp
    BmsStatus_t status;
    if (summary.communication)
    {
        status = bqRead(STACK_READ_VAL, 0, 0x0530, 1, 12, &resp, 20);
        if (status != BMS_OK){} // do smth
        markOk();
        for (int i = 1; i <= 12; i++)
        {
            if (resp.valid[i])
                registers.comm1[i] = resp.data[i][0];
            // else do something
        }

        status = bqRead(STACK_READ_VAL, 0, 0x0531, 1, 12, &resp, 20);
        if (status != BMS_OK){} // do smth
        markOk();   
        for (int i = 1; i <= 12; i++)
            {
                if (resp.valid[i])
                    registers.comm2[i] = resp.data[i][0];
                // else do something
            }

        status = bqRead(STACK_READ_VAL, 0, 0x0532, 1, 12, &resp, 20);
        if (status != BMS_OK){} // do smth
        markOk();  
        for (int i = 1; i <= 12; i++)
            {
                if (resp.valid[i])
                    registers.comm3[i] = resp.data[i][0];
                // else do something
            }
    }

    if (summary.otp)
    {
        status = bqRead(STACK_READ_VAL, 0, 0x0535, 1, 12, &resp, 20);
        if (status != BMS_OK){} // do smth
        markOk(); 
        for (int i = 1; i <= 12; i++)
            {
                if (resp.valid[i])
                    registers.otp[i] = resp.data[i][0];
                // else do something
            }
    }

    if (summary.system)
    {
        status = bqRead(STACK_READ_VAL, 0, 0x0536, 1, 12, &resp, 20);
        if (status != BMS_OK){} // do smth
        markOk(); 
        for (int i = 1; i <= 12; i++)
            {
                if (resp.valid[i])
                    registers.sys[i] = resp.data[i][0];
                // else do something
            }
    }

    if (summary.protection)
    {
        status = bqRead(STACK_READ_VAL, 0, 0x053A, 1, 12, &resp, 20);
        if (status != BMS_OK){} // do smth
        markOk();
            for (int i = 1; i <= 12; i++)
            {
                if (resp.valid[i])
                    registers.prot1[i] = resp.data[i][0];
                // else do something
            }

        status = bqRead(STACK_READ_VAL, 0, 0x053B, 1, 12, &resp, 20);
        if (status != BMS_OK){} // do smth
        markOk();
            for (int i = 1; i <= 12; i++)
            {
                if (resp.valid[i])
                    registers.prot2[i] = resp.data[i][0];
                // else do something
            }
    }

    if (summary.overVoltageUnderVoltage)
    {
        status = bqRead(STACK_READ_VAL, 0, 0x053C, 1, 12, &resp, 20);
        if (status != BMS_OK){} // do smth
        markOk();
        for (int i = 1; i <= 12; i++)
        {
            if (resp.valid[i])
                registers.ov1[i] = resp.data[i][0];
            // else do something
        }

        status = bqRead(STACK_READ_VAL, 0, 0x053D, 1, 12, &resp, 20);
        if (status != BMS_OK){} // do smth
            markOk();
        for (int i = 1; i <= 12; i++)
        {
            if (resp.valid[i])
                registers.ov2[i] = resp.data[i][0];
            // else do something
        }

        status = bqRead(STACK_READ_VAL, 0, 0x053E, 1, 12, &resp, 20);
        if (status != BMS_OK){} // do smth
            markOk();
        for (int i = 1; i <= 12; i++)
        {
            if (resp.valid[i])
                registers.uv1[i] = resp.data[i][0];
            // else do something
        }

        status = bqRead(STACK_READ_VAL, 0, 0x053F, 1, 12, &resp, 20);
        if (status != BMS_OK){} // do smth
            markOk();
        for (int i = 1; i <= 12; i++)
        {
            if (resp.valid[i])
                registers.uv2[i] = resp.data[i][0];
            // else do something
        }
    }

    if (summary.overTempUnderTemp)
    {
        status = bqRead(STACK_READ_VAL, 0, 0x0540, 1, 12, &resp, 20);
        if (status != BMS_OK){} // do smth
            markOk();
        for (int i = 1; i <= 12; i++)
        {
            if (resp.valid[i])
                registers.ot[i] = resp.data[i][0];
            // else do something
        }

        status = bqRead(STACK_READ_VAL, 0, 0x0541, 1, 12, &resp, 20);
        if (status != BMS_OK){} // do smth
            markOk();
        for (int i = 1; i <= 12; i++)
        {
            if (resp.valid[i])
                registers.ut[i] = resp.data[i][0];
            // else do something
        }
    }

    if (summary.comparatorADC)
    {
        status = bqRead(STACK_READ_VAL, 0, 0x0543, 1, 12, &resp, 20);
        if (status != BMS_OK){} // do smth
            markOk();
        for (int i = 1; i <= 12; i++)
        {
            if (resp.valid[i])
                registers.comp_gpio[i] = resp.data[i][0];
            // else do something
        }

        status = bqRead(STACK_READ_VAL, 0, 0x0545, 1, 12, &resp, 20);
        if (status != BMS_OK){} // do smth
            markOk();
        for (int i = 1; i <= 12; i++)
        {
            if (resp.valid[i])
                registers.comp_vccb1[i] = resp.data[i][0];
            // else do something
        }

        status = bqRead(STACK_READ_VAL, 0, 0x0546, 1, 12, &resp, 20);
        if (status != BMS_OK){} // do smth
            markOk();
        for (int i = 1; i <= 12; i++)
        {
            if (resp.valid[i])
                registers.comp_vccb2[i] = resp.data[i][0];
            // else do something
        }

        status = bqRead(STACK_READ_VAL, 0, 0x0548, 1, 12, &resp, 20);
        if (status != BMS_OK){} // do smth
            markOk();
        for (int i = 1; i <= 12; i++)
        {
            if (resp.valid[i])
                registers.comp_vcow1[i] = resp.data[i][0];
            // else do something
        }

        status = bqRead(STACK_READ_VAL, 0, 0x0549, 1, 12, &resp, 20);
        if (status != BMS_OK){} // do smth
            markOk();
        for (int i = 1; i <= 12; i++)
        {
            if (resp.valid[i])
                registers.comp_vcow2[i] = resp.data[i][0];
            // else do something
        }

        status = bqRead(STACK_READ_VAL, 0, 0x054B, 1, 12, &resp, 20);
        if (status != BMS_OK){} // do smth
            markOk();
        for (int i = 1; i <= 12; i++)
        {
            if (resp.valid[i])
                registers.comp_cbow1[i] = resp.data[i][0];
            // else do something
        }

        status = bqRead(STACK_READ_VAL, 0, 0x054C, 1, 12, &resp, 20);
        if (status != BMS_OK){} // do smth
            markOk();
        for (int i = 1; i <= 12; i++)
        {
            if (resp.valid[i])
                registers.comp_cbow2[i] = resp.data[i][0];
            // else do something
        }

        status = bqRead(STACK_READ_VAL, 0, 0x054E, 1, 12, &resp, 20);
        if (status != BMS_OK){} // do smth
            markOk();
        for (int i = 1; i <= 12; i++)
        {
            if (resp.valid[i])
                registers.comp_cbfet1[i] = resp.data[i][0];
            // else do something
        }

        status = bqRead(STACK_READ_VAL, 0, 0x054F, 1, 12, &resp, 20);
        if (status != BMS_OK){} // do smth
            markOk();
        for (int i = 1; i <= 12; i++)
        {
            if (resp.valid[i])
                registers.comp_cbfet2[i] = resp.data[i][0];
            // else do something
        }

        status = bqRead(STACK_READ_VAL, 0, 0x0550, 1, 12, &resp, 20);
        if (status != BMS_OK){} // do smth
            markOk();
        for (int i = 1; i <= 12; i++)
        {
            if (resp.valid[i])
                registers.comp_misc[i] = resp.data[i][0];
            // else do something
        }
    }

    if (summary.power)
    {
        status = bqRead(STACK_READ_VAL, 0, 0x0552, 1, 12, &resp, 20);
        if (status != BMS_OK){} // do smth
            markOk();
        for (int i = 1; i <= 12; i++)
        {
            if (resp.valid[i])
                registers.pwr1[i] = resp.data[i][0];
            // else do something
        }

        status = bqRead(STACK_READ_VAL, 0, 0x0553, 1, 12, &resp, 20);
        if (status != BMS_OK){} // do smth
            markOk();
        for (int i = 1; i <= 12; i++)
        {
            if (resp.valid[i])
                registers.pwr2[i] = resp.data[i][0];
            // else do something
        }

        status = bqRead(STACK_READ_VAL, 0, 0x0554, 1, 12, &resp, 20);
        if (status != BMS_OK){} // do smth
            markOk();
        for (int i = 1; i <= 12; i++)
        {
            if (resp.valid[i])
                registers.pwr3[i] = resp.data[i][0];
            // else do something
        }
    }

    return status;
}
