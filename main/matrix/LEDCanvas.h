/**
 * @author liushuai
 * @date 2022-02-16
 */
#pragma once

#include "LedMatrix.h"
#include "gfx/Adafruit_GFX.h"

class LEDCanvas : public GFXcanvas1 {
 public:
  LEDCanvas(LedMatrix* ledMatrix, uint16_t w, uint16_t h);

  void display();

 private:
  LedMatrix* ledMatrix;
};
