#pragma once

#include <Arduino.h>
#include <SPI.h>

#include "ESP32_CAN.h"

class SPI_CAN {
    public:
        void init();
        bool begin();
        bool write(const CAN_message_t &msg);
        void printStatus();
        bool read(CAN_message_t &msg);
    private:
        SPIClass _spi;
        bool pollReg(uint8_t addr, uint8_t mask, uint8_t expected, uint32_t timeout_ms);
};