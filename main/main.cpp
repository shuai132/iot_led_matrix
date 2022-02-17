#include <esp_log.h>
#include <esp_random.h>
#include <hal/gpio_types.h>

#include <nvs_handle.hpp>
#include <thread>

#include "esp_misc.h"
#include "img/bilibili.h"
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

static void show_loading() {
  ledCanvas->fillScreen(0);
  ledCanvas->setCursor(0, 1);
  ledCanvas->print("BILI");
  ledCanvas->drawBitmap(24, 0, bili_tv_data1, bili_tv_width, bili_tv_height, 1);
  ledCanvas->display();

  for (int i = 0; i < 32; ++i) {
    ledCanvas->drawRect(0, 9, 32, 6, 1);
    ledCanvas->fillRect(0, 9, i + 1, 6, 1);
    ledCanvas->display();
    delay_ms(2000 / 32);
  }
}

static void update_time_ui() {
  static int lastSecond = -1;
  tm* time_now;
  /// get time
  {
    time_t timer;
    time(&timer);
    time_now = localtime(&timer);
    // avoid unnecessary screen update
    if (time_now->tm_sec == lastSecond) {
      delay_ms(10);
      return;
    }
    lastSecond = time_now->tm_sec;
  }

  ledCanvas->fillScreen(0);

  /// up screen
  {
    char time_str[8];
    // hour
    sprintf(time_str, "%02d", time_now->tm_hour);
    ledCanvas->setCursor(2, 1);
    ledCanvas->print(time_str);
    // '::'
    ledCanvas->drawPixel(15, 3, 1);
    ledCanvas->drawPixel(16, 3, 1);
    ledCanvas->drawPixel(15, 5, 1);
    ledCanvas->drawPixel(16, 5, 1);
    // min
    sprintf(time_str, "%02d", time_now->tm_min);
    ledCanvas->setCursor(19, 1);
    ledCanvas->print(time_str);
  }
  /// bottom screen
  {
    // small tv animation
    auto tv_data = lastSecond % 2 ? bili_tv_data1 : bili_tv_data2;
    ledCanvas->drawBitmap(0, 8, tv_data, bili_tv_width, bili_tv_height, 1);

    // diy animation
    uint8_t fullNum = lastSecond / 8;
    if (fullNum != 0) {
      ledCanvas->fillRect(8, 15 - fullNum + 1, 8, fullNum, 1);
    }
    ledCanvas->drawLine(8, 15 - fullNum, 8 + lastSecond % 8, 15 - fullNum, 1);

    // second display
    ledCanvas->setCursor(19, 9);
    char time_str[8];
    sprintf(time_str, "%02d", lastSecond);
    ledCanvas->print(time_str);
  }

  ledCanvas->display();
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
  show_loading();

  for (;;) {
    update_time_ui();
  }
}
