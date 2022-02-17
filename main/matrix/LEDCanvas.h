/**
 * @author liushuai
 * @date 2022-02-16
 */
#pragma once

#include <memory>

#include "LedMatrix.h"
#include "gfx/Adafruit_GFX.h"

class LEDCanvas : public GFXcanvas1 {
 public:
  LEDCanvas(std::shared_ptr<LedMatrix> ledMatrix, uint16_t w, uint16_t h);
  virtual ~LEDCanvas();

  void display();

 private:
  std::shared_ptr<LedMatrix> ledMatrix;
};
