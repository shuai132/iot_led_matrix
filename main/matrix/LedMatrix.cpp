#include "LedMatrix.h"

#include "driver/gpio.h"

// the opcodes for the MAX7221 and MAX7219
#define OP_NOOP 0
#define OP_DIGIT0 1
#define OP_DIGIT1 2
#define OP_DIGIT2 3
#define OP_DIGIT3 4
#define OP_DIGIT4 5
#define OP_DIGIT5 6
#define OP_DIGIT6 7
#define OP_DIGIT7 8
#define OP_DECODEMODE 9
#define OP_INTENSITY 10
#define OP_SCANLIMIT 11
#define OP_SHUTDOWN 12
#define OP_DISPLAYTEST 15

void LedMatrix::initPins() const {
  gpio_config_t io_conf = {
      .pin_bit_mask = (1ULL << pinDIN) | (1ULL << pinCLK) | (1ULL << pinCS),
      .mode = GPIO_MODE_OUTPUT,
  };
  gpio_config(&io_conf);
  setPin((gpio_num_t)pinCS, 1);
}

void LedMatrix::setPin(int pin, int value) { gpio_set_level((gpio_num_t)pin, value); }

void LedMatrix::spiTransfer(int dev, volatile uint8_t opcode, volatile uint8_t data) {
  // create an array with the data to shift out
  int offset = dev * 2;
  int maxBytes = maxDevices * 2;

  for (int i = 0; i < maxBytes; i++) spiData[i] = (uint8_t)0;
  // put our device data into the array
  spiData[offset + 1] = opcode;
  spiData[offset] = data;
  // enable the line
  setPin(pinCS, 0);

  // now shift out the data
  for (int i = maxBytes; i > 0; i--) {
    uint8_t byteData = spiData[i - 1];
    for (int j = 0; j < 8; j++) {
      setPin(pinCLK, 0);
      setPin(pinDIN, byteData & 0x80);
      byteData = byteData << 1;
      setPin(pinCLK, 1);
    }
  }
  // latch the data onto the display
  setPin(pinCS, 1);
}

LedMatrix::LedMatrix(int dataPin, int clkPin, int csPin, int numDevices) : pinDIN(dataPin), pinCLK(clkPin), pinCS(csPin) {
  initPins();

  if (numDevices <= 0 || numDevices > 8) numDevices = 8;
  maxDevices = numDevices;

  for (int i = 0; i < maxDevices; i++) {
    spiTransfer(i, OP_DISPLAYTEST, 0);
    // scanlimit is set to max on startup
    setScanLimit(i, 7);
    // decode is done in source
    spiTransfer(i, OP_DECODEMODE, 0);
    clearDisplay(i);
    // we go into shutdown-mode on startup
    shutdown(i, true);
  }
}

int LedMatrix::getDeviceCount() const { return maxDevices; }

void LedMatrix::shutdown(int dev, bool state) {
  if (dev < 0 || dev >= maxDevices) return;
  spiTransfer(dev, OP_SHUTDOWN, !state);
}

void LedMatrix::setScanLimit(int dev, int limit) {
  if (dev < 0 || dev >= maxDevices) return;
  if (limit >= 0 && limit < 8) spiTransfer(dev, OP_SCANLIMIT, limit);
}

void LedMatrix::setIntensity(int dev, int intensity) {
  if (dev < 0 || dev >= maxDevices) return;
  if (intensity >= 0 && intensity < 16) spiTransfer(dev, OP_INTENSITY, intensity);
}

void LedMatrix::clearDisplay(int dev) {
  if (dev < 0 || dev >= maxDevices) return;
  int offset = dev * 8;
  for (int i = 0; i < 8; i++) {
    status[offset + i] = 0;
    spiTransfer(dev, i + 1, status[offset + i]);
  }
}

void LedMatrix::setLed(int dev, int row, int column, bool state) {
  if (dev < 0 || dev >= maxDevices) return;
  if (row < 0 || row > 7 || column < 0 || column > 7) return;
  int offset = dev * 8;
  uint8_t val = 0B10000000 >> column;
  if (state)
    status[offset + row] = status[offset + row] | val;
  else {
    val = ~val;
    status[offset + row] = status[offset + row] & val;
  }
  spiTransfer(dev, row + 1, status[offset + row]);
}

void LedMatrix::setRow(int dev, int row, uint8_t value) {
  if (dev < 0 || dev >= maxDevices) return;
  if (row < 0 || row > 7) return;
  int offset = dev * 8;
  status[offset + row] = value;
  spiTransfer(dev, row + 1, status[offset + row]);
}

void LedMatrix::setColumn(int dev, int col, uint8_t value) {
  if (dev < 0 || dev >= maxDevices) return;
  if (col < 0 || col > 7) return;
  for (int row = 0; row < 8; row++) {
    uint8_t val = value >> (7 - row);
    val = val & 0x01;
    setLed(dev, row, col, val);
  }
}
