#include <Arduino.h>
#include "bq_comm.h"
#include "bq_balance.h"

static bool addrOk = false;
static bool balOk = false;
static uint32_t startMs = 0;
static bool stopped = false;

void setup(){
    Serial.begin(115200);
    delay(2000);
    Serial.printf("\n=== bring-up + auto balance test (%d x BQ79616) ===\n", NUM_STACK_DEVICES);

    bqUartInit();
    delay(10);
    bq_wake();
    addrOk = bq_auto_address();
    Serial.printf("bq_auto_address(): %s\n", addrOk ? "PASS" : "FAIL");
    if (!addrOk) return;

    // balance cell 1 and 3 on every board for 30s
    balOk = bq_auto_balanceAll(0x005, CB_TIMER_30S);
    Serial.printf("bq_auto_balance_all(): %s\n", balOk ? "PASS" : "FAIL");

    startMs = millis();
}

void loop() {
    if (!addrOk) return;

    static uint32_t last = 0;
    if (millis() - last < 1000) return;
    last = millis();

    uint32_t t = (millis() - startMs) / 1000;
    Serial.printf("\n--- t = %lus ---\n", (unsigned long)t);
    bq_balance_PrintStatus();

    // stop to test bq_balanceStop()
    if (!stopped && t >= 20) {
        bq_balanceStop();
        stopped = true;
        Serial.println("bq_balance_stop() sent");
    }
}