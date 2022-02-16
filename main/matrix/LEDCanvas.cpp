#include "LEDCanvas.h"

LEDCanvas::LEDCanvas(LedMatrix* ledMatrix, uint16_t w, uint16_t h) : GFXcanvas1(w, h), ledMatrix(ledMatrix) {}

void LEDCanvas::display() {
  int devNum = ledMatrix->getDeviceCount();
  uint8_t* buffer = getBuffer();
  for (int row = 0; row < 8; row++) {
    for (int dev = 0; dev < devNum; ++dev) {
      ledMatrix->setRow(devNum - dev - 1, row, buffer[devNum * row + dev]);
    }
  }
}
