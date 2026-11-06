#include <Arduino.h>
#include "ESP32_CAN.h"

#include "fsm.h"
#include "bq_comm.h"


#ifndef LED_BUILTIN
#define LED_BUILTIN 21
#endif
unsigned long prevMillis = 0;

// CAN messages
ESP32_CAN Can;
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
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
    
    Serial.begin(115200);
    delay(2000);                       

    bqUartInit();
    delay(10);

    bq_wake();

    addrOk = bq_auto_address();
    Serial.printf("bq_auto_address(): %s\n", addrOk ? "PASS" : "FAIL");
}

void loop() {
    unsigned long now = millis();
    if (now - prevMillis >= 500) {
        prevMillis = now;
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    }
}