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

#define BQ_SERIAL      Serial1
#define TEST_DEV       0x00      // BQ79600 bridge
#define TEST_REG       0x2120    // Bridge_PARTID (from TI sample bq796xx_spiSlave.h) - confirm in datasheet

static void printHex(const uint8_t *b, size_t n) {
    for (size_t i = 0; i < n; i++) Serial.printf("%02X ", b[i]);
    Serial.println();
}

void setup() {
    Serial.begin(115200);
    delay(2000);                       // give USB-CDC time to enumerate
    Serial.println("\n=== BQ79600 wake/read test ===");
 
    bqUartInit();
    delay(10);
 
    bq_wake();
}

void loop() {
    static uint32_t count = 0;
    uint8_t tx[16];
    uint8_t rx[64];
    size_t  got = 0;
 
    // Build and send: single read, 1 byte (bytes_request is "bytes - 1", so 0)
    uint8_t txLen = build_read_frame(tx, SINGLE_READ_VAL, TEST_DEV, TEST_REG, 0);
    bqFlushRx();
    sendFrame(tx, txLen);
    BQ_SERIAL.flush();                 // wait until the last bit has left
 
    // Collect whatever shows up for up to 20 ms
    uint32_t t0 = millis();
    while ((millis() - t0) < 20 && got < sizeof(rx)) {
        while (BQ_SERIAL.available() && got < sizeof(rx)) {
            rx[got++] = (uint8_t)BQ_SERIAL.read();
        }
    }
 
    Serial.printf("[%lu] TX: ", (unsigned long)count++);
    printHex(tx, txLen);
    Serial.printf("      RX (%u bytes): ", (unsigned)got);
    if (got) printHex(rx, got); else Serial.println("<nothing>");
 
    if (got == 0) {
        Serial.println("      -> no reply (check power, wiring, wake ping, UART mode)");
    } else if (got == 7 && crc_compute(rx, got) == 0) {
        Serial.printf("      -> CRC OK. dev=%u reg=0x%02X%02X data=0x%02X\n",
                      rx[1] & 0x3F, rx[2], rx[3], rx[4]);
    } else if (got >= txLen && memcmp(rx, tx, txLen) == 0) {
        Serial.println("      -> RX is an echo of TX (RX/TX shorted or looped back)");
    } else {
        Serial.println("      -> got bytes but not a valid 7-byte reply (CRC/length wrong)");
    }
 
    delay(500);
}