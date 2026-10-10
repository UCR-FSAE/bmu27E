#include <Arduino.h>
#include "ESP32_CAN.h"

// CAN messages
static CAN_message_t CAN_outMsg1;
static CAN_message_t CAN_outMsg2;

char msgBuf[] = {'h','e','l','l','o','b','r','o'};

ESP32_CAN Can;

void setup() {
    Serial.begin(9600);

    if (!Can.begin()) {
        Serial.println("CAN init failed!");
        while (1);
    }

    CAN_outMsg1.len = CAN_outMsg2.len = 8;
    CAN_outMsg1.id = CAN_outMsg2.id = 0xC0;

    memcpy(CAN_outMsg1.buf, msgBuf, 8);
    memcpy(CAN_outMsg2.buf, msgBuf, 8);

    Serial.println("Setup complete. Sending data over CAN.");
}

void loop() {
    Serial.println(Can.write(CAN_outMsg1) ? "MSG1 sent" : "MSG1 FAILED");
    Serial.println(Can.write(CAN_outMsg2) ? "MSG2 sent" : "MSG2 FAILED");
    Can.printStatus();
    // CAN TX = D4, RX = D3
    delay(100);
}