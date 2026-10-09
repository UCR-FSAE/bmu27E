#include <Arduino.h>
#include "ESP32_CAN.h"

// CAN messages
static CAN_message_t CAN_outMsg1;
static CAN_message_t CAN_outMsg2;

char msgBuf[] = {'h','e','l','l','o','b','r','o'};

ESP32_CAN Can;

void setup() {
    if (!Can.begin()) {
        Serial.println("CAN init failed!");
        while (1);
    }

    CAN_outMsg1.len = CAN_outMsg2.len = 8;

    Serial.println("Setup complete. Sending data over CAN.");
}

void loop() {
    for (int i = 0; i < 8; i++) {
        CAN_outMsg1.buf[i] = msgBuf[i];
        CAN_outMsg2.buf[i] = msgBuf[i];
    }

    Serial.println(Can.write(CAN_outMsg1) ? "MSG1 sent" : "MSG1 FAILED");
    Serial.println(Can.write(CAN_outMsg2) ? "MSG2 sent" : "MSG2 FAILED");

    // CAN TX = D4, RX = D3
}