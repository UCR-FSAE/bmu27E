#pragma once
#include <Arduino.h>
#include <SPI.h>

enum MiniCANSpeed : uint8_t {
  CAN_100K, CAN_125K, CAN_250K, CAN_500K, CAN_1000K
};

class MiniCAN {
public:
  // Assumes the 8 MHz crystal used on the common MCP2515 module.
  // Call SPI.begin() (or SPI.begin(sck, miso, mosi, cs)) before begin().
  bool begin(uint8_t csPin, MiniCANSpeed speed, SPIClass &spi = SPI);

  // Queue one frame on TXB0. Returns false if the buffer stays busy (no ACK / bus problem).
  bool send(uint32_t id, const uint8_t *data, uint8_t len, bool extended = false);

private:
  SPIClass *_spi;
  uint8_t _cs;
  void _write(uint8_t addr, const uint8_t *buf, uint8_t n);
  uint8_t _read(uint8_t addr);
  void _bitModify(uint8_t addr, uint8_t mask, uint8_t val);
};