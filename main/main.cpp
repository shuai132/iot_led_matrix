#include <esp_log.h>
#include <hal/gpio_types.h>

#include <nvs_handle.hpp>
#include <thread>

#include "esp_misc.h"
#include "matrix/LEDCanvas.h"
#include "wifi/smartconfig.h"
#include "wifi/sntp.h"
#include "wifi/wifi_station.h"

const static char* TAG = "MAIN";
const static char* NS_NAME_WIFI = "wifi";

static std::shared_ptr<LedMatrix> ledMatrix;
static std::shared_ptr<LEDCanvas> ledCanvas;

static void on_wifi_connected(const char* ip) { sntp_obtain_time(); }

static void start_wifi_task() {
  static auto start_smartconfig = [] {
    start_smartconfig_task(
        [](const WiFiInfo& info) {
          ESP_LOGI(TAG, "WiFiInfo:%s %s", info.ssid.c_str(), info.passwd.c_str());
          auto nvs = nvs::open_nvs_handle(NS_NAME_WIFI, NVS_READWRITE);
          nvs->set_string("ssid", info.ssid.c_str());
          nvs->set_string("passwd", info.passwd.c_str());
          nvs->set_item("configed", true);
          nvs->commit();
        },
        [](ConnectState state, void* data) {
          ESP_LOGI(TAG, "ConnectState:%d", (int)state);
          if (state == ConnectState::Connected) {
            on_wifi_connected((char*)data);
          }
        });
  };

  auto nvs = nvs::open_nvs_handle(NS_NAME_WIFI, NVS_READWRITE);
  bool configed = false;
  nvs->get_item("configed", configed);
  if (configed) {
    char ssid[32];
    char passwd[64];
    nvs->get_string("ssid", ssid, sizeof(ssid));
    nvs->get_string("passwd", passwd, sizeof(passwd));
    wifi_station_init(
        ssid, passwd,
        [](const char* ip) {
          ESP_LOGI(TAG, "wifi_station connected");
          on_wifi_connected(ip);
        },
        [] { start_smartconfig(); });
  } else {
    start_smartconfig();
  }
}

extern "C" void app_main() {
  ESP_LOGI(TAG, "init");

  esp_init();
  sntp_env_init();
  start_wifi_task();

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
  delay_ms(1000);

  int lastSecond = -1;

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
  for (;;) {
    char time_str1[8];
    char time_str2[8];
    {
      time_t timer;
      time(&timer);
      auto tm = localtime(&timer);
      // avoid unnecessary screen update
      if (tm->tm_sec == lastSecond) {
        delay_ms(10);
        continue;
      }
      lastSecond = tm->tm_sec;
      strftime(time_str1, sizeof(time_str1), "%H:%M", tm);
      strftime(time_str2, sizeof(time_str2), "%M:%S", tm);
    }

    ledCanvas->fillScreen(0);

    ledCanvas->setCursor(1, 1);
    ledCanvas->print(time_str1);

    ledCanvas->setCursor(1, 9);
    ledCanvas->print(time_str2);

    ledCanvas->display();
  }
#pragma clang diagnostic pop
}
