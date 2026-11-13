#include "SPI_CAN.h"

// SPI commands
#define CMD_RESET   0xC0
#define CMD_READ    0x03
#define CMD_WRITE   0x02
#define CMD_LOAD_TX 0x40   // TXB0SIDH
#define CMD_RTS_TX0 0x81
#define CMD_BITMOD  0x05
#define CMD_ZERO    0x00

// Registers
#define REG_CNF3     0x28
#define REG_TXB0CTRL 0x30
#define REG_CANSTAT  0x0E
#define REG_CANCTRL  0x0F

static const SPISettings kSpi(8000000, MSBFIRST, SPI_MODE0);

const uint8_t CS_PIN = 11;

void SPI_CAN::init() {
    _spi.begin();
    pinMode(CS_PIN, OUTPUT);
    digitalWrite(CS_PIN, HIGH); 
}

bool pollReg(uint8_t addr, uint8_t mask, uint8_t expected, uint32_t timeout_ms) {
    uint32_t now = millis();

    digitalWrite(CS_PIN, LOW);
    _spi.transfer(CMD_READ);
    _spi.transfer(addr);
    uint8_t expected = _spi.transfer(0x00);
}

bool SPI_CAN::begin() {
    _spi.beginTransaction(kSpi);

    // send reset to set config mode
    digitalWrite(CS_PIN, LOW);
    _spi.transfer(CMD_RESET);
    digitalWrite(CS_PIN, HIGH);
    delay(5);

    // check for 
    
    // addr 0x*F
    // set REQOP[2:0] bits to ‘100’ -> enter config mode
        /* CNF1, CNF2, CNF3 registers
        • TXRTSCTRL register
        • Filter registers
        • Mask registers
        */
    // do config

    // set REQOP[2:0] to '000' = Normal operation
    digitalWrite(CS_PIN, HIGH);
    return true;
}