#pragma once

#include <cstdint>

class LedMatrix {
 private:
  /* Data is shifted out of this pin*/
  int pinDIN;
  /* The clock is signaled on this pin */
  int pinCLK;
  /* This one is driven LOW for chip selection */
  int pinCS;

  /* The array for shifting the data to the devices */
  uint8_t spiData[16]{};
  /* We keep track of the led-status for all 8 devices in this array */
  uint8_t status[64]{};
  /* The maximum number of devices we use */
  int maxDevices;

  /* init gpio to output mode */
  void initPins() const;

  /* set pin value */
  static void setPin(int pin, int value);

  /* Send out a single command to the device */
  void spiTransfer(int dev, uint8_t opcode, uint8_t data);

 public:
  /*
   * Create a new controller
   * Params :
   * dataPin		pin on the Arduino where data gets shifted out
   * clockPin		pin for the clock
   * csPin		pin for selecting the device
   * numDevices	maximum number of devices that can be controlled
   */
  LedMatrix(int dataPin, int clkPin, int csPin, int numDevices = 1);

  /*
   * Gets the number of devices attached to this LedControl.
   * Returns :
   * int	the number of devices on this LedControl
   */
  int getDeviceCount() const;

  /*
   * Set the shutdown (power saving) mode for the device
   * Params :
   * dev	The address of the display to control
   * state	If true the device goes into power-down mode. Set to false for
   * normal operation.
   */
  void shutdown(int dev, bool state);

  /*
   * Set the number of digits (or rows) to be displayed.
   * See datasheet for sideeffects of the scanlimit on the brightness
   * of the display.
   * Params :
   * dev	address of the display to control
   * limit	number of digits to be displayed (1..8)
   */
  void setScanLimit(int dev, int limit);

  /*
   * Set the brightness of the display.
   * Params:
   * dev		the address of the display to control
   * intensity	the brightness of the display. (0..15)
   */
  void setIntensity(int dev, int intensity);

  /*
   * Switch all Leds on the display off.
   * Params:
   * dev	address of the display to control
   */
  void clearDisplay(int dev);

  /*
   * Set the status of a single Led.
   * Params :
   * dev	address of the display
   * row	the row of the Led (0..7)
   * col	the column of the Led (0..7)
   * state	If true the led is switched on,
   *		if false it is switched off
   */
  void setLed(int dev, int row, int col, bool state);

  /*
   * Set all 8 Led's in a row to a new state
   * Params:
   * dev	address of the display
   * row	row which is to be set (0..7)
   * value	each bit set to 1 will light up the
   *		corresponding Led.
   */
  void setRow(int dev, int row, uint8_t value);

  /*
   * Set all 8 Led's in a column to a new state
   * Params:
   * dev	address of the display
   * col	column which is to be set (0..7)
   * value	each bit set to 1 will light up the
   *		corresponding Led.
   */
  void setColumn(int dev, int col, uint8_t value);
};
