#include "LEDCanvas.h"

#include <utility>

LEDCanvas::LEDCanvas(std::shared_ptr<LedMatrix> ledMatrix, uint16_t w, uint16_t h) : GFXcanvas1(w, h), ledMatrix(std::move(ledMatrix)) {}

LEDCanvas::~LEDCanvas() = default;

void LEDCanvas::display() {
  int devNum = ledMatrix->getDeviceCount();
  int devNumHorizon = _width / 8;
  uint8_t* buffer = getBuffer();
  for (int h = 0; h < _height; ++h) {
    for (int w = 0; w < devNumHorizon; ++w) {
      ledMatrix->setRow(devNum - (h / 8 * devNumHorizon + w) - 1, h % 8, buffer[h * devNumHorizon + w]);
    }
  }
}
