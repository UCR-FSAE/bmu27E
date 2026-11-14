#include "SPI_CAN.h"

// SPI commands
#define CMD_RESET   0xC0
#define CMD_READ    0x03
#define CMD_WRITE   0x02
#define CMD_LOAD_TX 0x40   // TXB0SIDH
#define CMD_RTS_TX0 0x81
#define CMD_STATUS  0xA0
#define CMD_BITMOD  0x05

// Registers
#define REG_CNF1     0x2A
#define REG_CNF2     0x29
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

bool SPI_CAN::readReg(uint8_t addr, uint8_t mask, uint8_t expected) {
    this->START_TX();
    _spi.transfer(CMD_READ);
    _spi.transfer(addr);
    uint8_t ret_val = _spi.transfer(0x00);
    this->END_TX();

    if ((ret_val & mask) == expected) { return true; }
    return false;
}

void SPI_CAN::START_TX() {
    _spi.beginTransaction(kSpi);
    digitalWrite(CS_PIN, LOW);
}

void SPI_CAN::END_TX() {
    digitalWrite(CS_PIN, HIGH);
    _spi.endTransaction();
}

bool SPI_CAN::begin() {
    // send reset to set config mode
    this->START_TX();
    _spi.transfer(CMD_RESET);
    this->END_TX();
    delay(5);

    // check for '0b100' in CANSTAT
    if (!this->readReg(REG_CANSTAT, 0xE0, 0x80)) {
        return false;
    }

    // config CNF1, CNF2, CNF3 registers
    
    this->START_TX();
    _spi.transfer(CMD_WRITE);
    _spi.transfer(REG_CNF1);
    _spi.transfer(0x00);
    this->END_TX();

    this->START_TX();
    _spi.transfer(CMD_WRITE);
    _spi.transfer(REG_CNF2);
    _spi.transfer(0x91);
    this->END_TX();

    this->START_TX();
    _spi.transfer(CMD_WRITE);
    _spi.transfer(REG_CNF3);
    _spi.transfer(0x01);
    this->END_TX();

    // set REQOP[2:0] to '0b000' = Normal operation
    this->START_TX();
    _spi.transfer(CMD_BITMOD);
    _spi.transfer(REG_CANCTRL);
    _spi.transfer(0xE0);
    _spi.transfer(0x00);
    this->END_TX();

    delay(5);

    if (!this->readReg(REG_CANSTAT, 0xE0, 0x00)) {
        return false;
    }

    return true;
}

bool SPI_CAN::write(const CAN_message_t &msg) {
    if (msg.len > 8) { return false; }
    if (msg.id > 0x7FF) { return false; }

    // check for free tx buffer
    this->START_TX();
    _spi.transfer(CMD_STATUS);
    uint8_t data_out = _spi.transfer(0x00);
    this->END_TX();

    // clear buffer if not free
    if (data_out & 0x04) {
        this->START_TX();
        _spi.transfer(CMD_BITMOD);
        _spi.transfer(REG_TXB0CTRL);
        _spi.transfer(0x08);
        _spi.transfer(0x00);
        this->END_TX();

        // verify clear buffer
        this->START_TX();
        _spi.transfer(CMD_STATUS);
        data_out = _spi.transfer(0x00);
        this->END_TX();
        if (data_out & 0x04) { return false; }
    }


    // load tx: SIDH, SIDL, EID8, EID0, DLC, buf
    this->START_TX();
    _spi.transfer(CMD_LOAD_TX);
    _spi.transfer((msg.id >> 3) & 0xFF);
    _spi.transfer((msg.id & 0x07) << 5);
    _spi.transfer(0x00);
    _spi.transfer(0x00);
    _spi.transfer(msg.len);

    for (int i = 0; i < msg.len; i++) {
        _spi.transfer(msg.buf[i]);
    }

    this->END_TX();

    // request to send 
    this->START_TX();
    _spi.transfer(CMD_RTS_TX0);
    this->END_TX();

    return true;
}