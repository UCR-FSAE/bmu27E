#include "MiniCAN.h"

// SPI commands
#define CMD_RESET   0xC0
#define CMD_READ    0x03
#define CMD_WRITE   0x02
#define CMD_LOAD_TX 0x40   // TXB0SIDH
#define CMD_RTS_TX0 0x81
#define CMD_BITMOD  0x05

// Registers
#define REG_CNF3     0x28
#define REG_TXB0CTRL 0x30
#define REG_CANSTAT  0x0E
#define REG_CANCTRL  0x0F

static const SPISettings kSpi(8000000, MSBFIRST, SPI_MODE0);

// CNF3, CNF2, CNF1 for 8 MHz oscillator
static const uint8_t kCnf[][3] = {
  {0x86, 0xB4, 0x01},  // 100k
  {0x85, 0xB1, 0x01},  // 125k
  {0x85, 0xB1, 0x00},  // 250k
  {0x82, 0x90, 0x00},  // 500k
  {0x80, 0x80, 0x00},  // 1M
};

void MiniCAN::_write(uint8_t addr, const uint8_t *buf, uint8_t n) {
  _spi->beginTransaction(kSpi);
  digitalWrite(_cs, LOW);
  _spi->transfer(CMD_WRITE);
  _spi->transfer(addr);
  while (n--) _spi->transfer(*buf++);
  digitalWrite(_cs, HIGH);
  _spi->endTransaction();
}

uint8_t MiniCAN::_read(uint8_t addr) {
  _spi->beginTransaction(kSpi);
  digitalWrite(_cs, LOW);
  _spi->transfer(CMD_READ);
  _spi->transfer(addr);
  uint8_t v = _spi->transfer(0);
  digitalWrite(_cs, HIGH);
  _spi->endTransaction();
  return v;
}

void MiniCAN::_bitModify(uint8_t addr, uint8_t mask, uint8_t val) {
  _spi->beginTransaction(kSpi);
  digitalWrite(_cs, LOW);
  _spi->transfer(CMD_BITMOD);
  _spi->transfer(addr);
  _spi->transfer(mask);
  _spi->transfer(val);
  digitalWrite(_cs, HIGH);
  _spi->endTransaction();
}

bool MiniCAN::begin(uint8_t csPin, MiniCANSpeed speed, SPIClass &spi) {
  _cs = csPin;
  _spi = &spi;
  pinMode(_cs, OUTPUT);
  digitalWrite(_cs, HIGH);

  // Software reset -> chip enters configuration mode
  _spi->beginTransaction(kSpi);
  digitalWrite(_cs, LOW);
  _spi->transfer(CMD_RESET);
  digitalWrite(_cs, HIGH);
  _spi->endTransaction();
  delay(10);

  // Expect CANSTAT = 0x80 (config mode) after reset; otherwise module not responding
  if ((_read(REG_CANSTAT) & 0xE0) != 0x80) return false;

  // Bit timing: writes CNF3, CNF2, CNF1 (consecutive addresses 0x28..0x2A)
  _write(REG_CNF3, kCnf[speed], 3);

  // Switch to normal mode and verify
  _bitModify(REG_CANCTRL, 0xE0, 0x00);
  uint32_t t = millis();
  while ((_read(REG_CANSTAT) & 0xE0) != 0x00) {
    if (millis() - t > 20) return false;
  }
  return true;
}

bool MiniCAN::send(uint32_t id, const uint8_t *data, uint8_t len, bool extended) {
  if (len > 8) len = 8;

  // Wait for TXB0 to be free (TXREQ = bit 3)
  uint32_t t = millis();
  while (_read(REG_TXB0CTRL) & 0x08) {
    if (millis() - t > 10) return false;
  }

  uint8_t hdr[5];
  if (extended) {
    hdr[0] = id >> 21;
    hdr[1] = ((id >> 13) & 0xE0) | 0x08 | ((id >> 16) & 0x03);
    hdr[2] = id >> 8;
    hdr[3] = id;
  } else {
    hdr[0] = id >> 3;
    hdr[1] = (id & 0x07) << 5;
    hdr[2] = 0;
    hdr[3] = 0;
  }
  hdr[4] = len;

  // Load ID, DLC and data in a single transaction, then request to send
  _spi->beginTransaction(kSpi);
  digitalWrite(_cs, LOW);
  _spi->transfer(CMD_LOAD_TX);
  for (uint8_t i = 0; i < 5; i++) _spi->transfer(hdr[i]);
  for (uint8_t i = 0; i < len; i++) _spi->transfer(data[i]);
  digitalWrite(_cs, HIGH);

  digitalWrite(_cs, LOW);
  _spi->transfer(CMD_RTS_TX0);
  digitalWrite(_cs, HIGH);
  _spi->endTransaction();
  return true;
}