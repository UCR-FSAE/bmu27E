#include <Arduino.h>
#include "ESP32_CAN.h"

#include "fsm.h"
#include "bq_comm.h"

ESP32_CAN Can;

// CAN messages
static CAN_message_t CAN_outMsg1;
static CAN_message_t CAN_outMsg2;

// ------------- BMS Types -------------
CurrentState    curState;
Inputs          in;
Outputs         out;

// Registers used for verification
#define BRIDGE_PARTID_REG   0x2120   // BQ79600 PARTID (silicon rev, 0x01 = A1 on your board)
#define BQ616_DIR0_ADDR_REG 0x0306   // device address
#define BQ616_PARTID_REG    0x0500   // 0x21 = BQ79616

static BqResponse resp;
static bool addrOk = false;

// Stack-read one byte from every 616 and check it against `expected`.
// expected == -1 means "expect the device's own address" (used for DIR0_ADDR).
static bool checkStack(const char *name, uint16_t reg, int expected, uint8_t mask) {
    int n = bqRead(STACK_READ_VAL, 0, reg, 1, NUM_STACK_DEVICES, &resp, 20);
    bool ok = (n == NUM_STACK_DEVICES);
    Serial.printf("  %s: got %d/%d replies\n", name, n, NUM_STACK_DEVICES);
    for (int a = 1; a <= NUM_STACK_DEVICES; a++) {
        int want = (expected < 0) ? a : expected;
        bool good = resp.valid[a] && (resp.data[a][0] & mask) == want;
        if (!good) {
            if (resp.valid[a])
                Serial.printf("    dev %2d: BAD (got 0x%02X, want 0x%02X)\n", a, resp.data[a][0], want);
            else
                Serial.printf("    dev %2d: NO REPLY\n", a);
        }
        ok &= good;
    }
    return ok;
}

void setup() {
    Serial.begin(115200);
    delay(2000);                       // give USB-CDC time to enumerate
    Serial.println("\n=== BQ79600 + 12x BQ79616 wake / auto-address test ===");

    bqUartInit();
    delay(10);

    bq_wake();

    addrOk = bq_auto_address();
    Serial.printf("bq_auto_address(): %s\n", addrOk ? "PASS" : "FAIL");
}

void loop() {
    static uint32_t count = 0;
    Serial.printf("\n--- check %lu ---\n", (unsigned long)count++);

    // Bridge: single read
    int nb = bqRead(SINGLE_READ_VAL, 0, BRIDGE_PARTID_REG, 1, 1, &resp, 20);
    bool bridgeOk = (nb == 1) && resp.valid[0];
    Serial.printf("  bridge PARTID: %s (0x%02X)\n", bridgeOk ? "OK" : "NO REPLY",
                  bridgeOk ? resp.data[0][0] : 0);

    // Stack: every 616 must answer with its own address, and with PARTID 0x21
    bool addrGood = checkStack("DIR0_ADDR", BQ616_DIR0_ADDR_REG, -1,   0x3F);
    bool idGood   = checkStack("PARTID   ", BQ616_PARTID_REG,    0x21, 0xFF);

    Serial.printf("RESULT: %s\n", (bridgeOk && addrGood && idGood) ? "ALL 12 + BRIDGE OK" : "PROBLEM");

    delay(1000);
}