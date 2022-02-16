#include <esp_log.h>
#include <hal/gpio_types.h>

#include <thread>

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
  std::this_thread::sleep_for(std::chrono::seconds(1));

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
  for (;;) {
    canvas.fillScreen(0);

    char time_str1[8];
    char time_str2[8];
    {
      time_t timer;
      time(&timer);
      auto tm = localtime(&timer);
      strftime(time_str1, sizeof(time_str1), "%H:%M", tm);
      strftime(time_str2, sizeof(time_str2), "%M:%S", tm);
    }

    canvas.setCursor(1, 1);
    canvas.print(time_str1);

    canvas.setCursor(33, 1);
    canvas.print(time_str2);

    canvas.display();
    std::this_thread::sleep_for(std::chrono::microseconds(300));
  }
#pragma clang diagnostic pop
}
