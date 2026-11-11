#include <Arduino.h>
#include "ESP32_CAN.h"

#include "fsm.h"
#include "bq_comm.h"
#include "MiniCan.h"


#ifndef LED_BUILTIN
#define LED_BUILTIN 21
#endif
unsigned long prevMillis = 0;

// CAN messages
ESP32_CAN Can;
static CAN_message_t CAN_outMsg1;
static CAN_message_t CAN_outMsg2;

const uint8_t CS_PIN = 11;
MiniCAN can2;

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
    BmsStatus_t st = bqRead(STACK_READ_VAL, 0, reg, 1, NUM_STACK_DEVICES, &resp, 20);
    bool ok = (st == BMS_OK);
    Serial.printf("  %s: got %d/%d replies (status %d, %d CRC errors)\n",
                  name, resp.num_valid, NUM_STACK_DEVICES, (int)st, resp.crc_errors);
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

    SPI.begin();
    if (!can2.begin(CS_PIN, CAN_500K)) {
        Serial.println("CAN init failed - check wiring/power");
    }

    Serial.println("CAN ready");
}

void loop() {
    unsigned long now = millis();
    if (now - prevMillis >= 500) {
        prevMillis = now;
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    }

    uint8_t data[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    bool ok = can2.send(0x123, data, 8);          // standard 11-bit ID
    Serial.println(ok ? "sent" : "tx busy (no ACK on bus?)");
    delay(500);
}