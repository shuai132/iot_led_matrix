#include <esp_log.h>
#include <hal/gpio_types.h>

#include "matrix/LEDCanvas.h"

const static char *TAG = "MAIN";

extern "C" void app_main() {
  ESP_LOGI(TAG, "init");

  LedMatrix led = LedMatrix(GPIO_NUM_8, GPIO_NUM_6, GPIO_NUM_7, 8);
  for (int i = 0; i < 8; i++) {
    led.shutdown(i, false);
    led.setIntensity(i, 1);
    led.clearDisplay(i);
  }

  LEDCanvas canvas(&led, 64, 8);
  canvas.setTextColor(1);
  canvas.setCursor(1, 1);
  canvas.print("Hello");
  canvas.setCursor(33, 1);
  canvas.print("World");
  canvas.display();
}
