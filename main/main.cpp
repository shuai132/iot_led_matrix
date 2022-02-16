#include <esp_log.h>
#include <hal/gpio_types.h>

#include <thread>

#include "matrix/LedMatrix.h"

const static char *TAG = "MAIN";

extern "C" void app_main() {
  ESP_LOGI(TAG, "init");

  LedMatrix led = LedMatrix(GPIO_NUM_8, GPIO_NUM_6, GPIO_NUM_7, 8);
  for (int i = 0; i < 8; i++) {
    led.shutdown(i, false);
    led.setIntensity(i, 1);
    led.clearDisplay(i);
  }

  led.setRow(0, 1, 0B10100000);
  led.setColumn(0, 1, 0B10100000);

  for (;;) {
    ESP_LOGI(TAG, "loop");
    for (int i = 0; i < 8; i++) {
      led.setRow(1, 0, 0x80 >> i);
      led.setColumn(1, 1, 0x80 >> i);
      std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }
  }
}
