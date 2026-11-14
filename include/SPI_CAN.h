#pragma once

#include <Arduino.h>
#include <SPI.h>

#include "ESP32_CAN.h"

class SPI_CAN {
    public:
        void init();
        bool begin();
        bool write(const CAN_message_t &msg);
        bool read(CAN_message_t &msg);
        void printStatus();
    private:
        SPIClass _spi;
        bool readReg(uint8_t addr, uint8_t mask, uint8_t expected);
        void START_TX();
        void END_TX();
};