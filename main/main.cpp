#include <esp_log.h>
#include <hal/gpio_types.h>

#include <thread>

#include "matrix/LEDCanvas.h"

const static char *TAG = "MAIN";

static std::shared_ptr<LedMatrix> ledMatrix;
static std::shared_ptr<LEDCanvas> ledCanvas;

extern "C" void app_main() {
  ESP_LOGI(TAG, "init");

  ledMatrix = std::make_shared<LedMatrix>(GPIO_NUM_8, GPIO_NUM_6, GPIO_NUM_7, 8);
  for (int i = 0; i < 8; i++) {
    ledMatrix->shutdown(i, false);
    ledMatrix->setIntensity(i, 1);
    ledMatrix->clearDisplay(i);
  }

  ledCanvas = std::make_shared<LEDCanvas>(ledMatrix, 32, 16);
  ledCanvas->setTextColor(1);
  ledCanvas->setCursor(1, 1);
  ledCanvas->print("Hello");
  ledCanvas->setCursor(1, 9);
  ledCanvas->print("World");
  ledCanvas->display();
  std::this_thread::sleep_for(std::chrono::seconds(1));

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
  for (;;) {
    ledCanvas->fillScreen(0);

    char time_str1[8];
    char time_str2[8];
    {
      time_t timer;
      time(&timer);
      auto tm = localtime(&timer);
      strftime(time_str1, sizeof(time_str1), "%H:%M", tm);
      strftime(time_str2, sizeof(time_str2), "%M:%S", tm);
    }

    ledCanvas->setCursor(1, 1);
    ledCanvas->print(time_str1);

    ledCanvas->setCursor(1, 9);
    ledCanvas->print(time_str2);

    ledCanvas->display();
    std::this_thread::sleep_for(std::chrono::microseconds(300));
  }
#pragma clang diagnostic pop
}
