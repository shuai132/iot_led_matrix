#include <driver/gpio.h>
#include <esp_log.h>
#include <esp_random.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <hal/gpio_types.h>

#include <nvs_handle.hpp>

#include "EventLoop.h"
#include "esp_misc.h"
#include "img/bilibili.h"
#include "matrix/LEDCanvas.h"
#include "utils/IntervalCall.hpp"
#include "wifi/smartconfig.h"
#include "wifi/sntp.h"
#include "wifi/wifi_station.h"

enum class BottomShowType {
  kSecond = 0,
  kYear,
  kMon,
};
#define BottomShowTypeNum 3

enum class TimeSettingType {
  kNone = 0,
  kMin,
  kHour,
  kDay,
  kMon,
  kYear,
};
#define TimeSettingTypeNum 6

#define BUTTON_UP GPIO_NUM_1
#define BUTTON_DOWN GPIO_NUM_2
#define BUTTON_SET GPIO_NUM_3
#define BUTTON_SW GPIO_NUM_4

static BottomShowType bottomShowType;
static TimeSettingType timeSettingType;

const static char* TAG = "MAIN";
const static char* NS_NAME_WIFI = "wifi";

static std::shared_ptr<LedMatrix> ledMatrix;
static std::shared_ptr<LEDCanvas> ledCanvas;
static EventLoop eventLoop;
static xQueueHandle gpio_evt_queue = xQueueCreate(8, 1);

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

  // show loading for 2 second
  for (int i = 1; i < 32; ++i) {
    ledCanvas->drawRoundRect(0, 9, 32, 6, 1, 1);
    ledCanvas->fillRoundRect(0, 9, i + 1, 6, 1, 1);
    ledCanvas->display();
    delay_ms(1000 * 2 / 32);
  }
}

static void config_button() {
  ESP_ERROR_CHECK(gpio_install_isr_service(0));
  gpio_num_t gpio_pins[] = {BUTTON_UP, BUTTON_DOWN, BUTTON_SET, BUTTON_SW};

  for (auto pin : gpio_pins) {
    gpio_config_t io_conf = {
        .pin_bit_mask = static_cast<uint64_t>(1 << pin),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE,
    };
    gpio_config(&io_conf);

    struct UserData {
      gpio_num_t pin{};
      IntervalCall intervalCall = IntervalCall(std::chrono::milliseconds(100));
    };
    auto data = new UserData();  // no need free!!!
    data->pin = pin;
    ESP_ERROR_CHECK(gpio_isr_handler_add(
        pin,
        [](void* args) IRAM_ATTR {
          auto data = static_cast<UserData*>(args);
          // avoid jitter
          if (!data->intervalCall.poll()) return;
          uint8_t value = data->pin;
          xQueueSendFromISR(gpio_evt_queue, &value, nullptr);
        },
        data));
  }
}

static void check_button() {
  uint8_t pin;
  if (!xQueueReceive(gpio_evt_queue, &pin, 0)) return;
  ESP_LOGI(TAG, "button: %d", pin);
  switch (pin) {
    case BUTTON_SW: {
      // switch show type
      static uint8_t count = 0;
      bottomShowType = static_cast<BottomShowType>(++count % BottomShowTypeNum);
    } break;
    case BUTTON_SET: {
      // switch time settings mode
      static uint8_t count = 0;
      timeSettingType = static_cast<TimeSettingType>(++count % TimeSettingTypeNum);
      if (timeSettingType == TimeSettingType::kNone) {
        bottomShowType = BottomShowType::kSecond;
      }
    } break;
    case BUTTON_UP:
    case BUTTON_DOWN: {
      int v = pin == BUTTON_UP ? 1 : -1;
      tm* time_now;
      time_t timer;
      time(&timer);
      time_now = localtime(&timer);
      switch (timeSettingType) {
        case TimeSettingType::kNone:
          break;
        case TimeSettingType::kYear:
          time_now->tm_year += v;
          break;
        case TimeSettingType::kMon:
          time_now->tm_mon += v;
          break;
        case TimeSettingType::kDay:
          time_now->tm_mday += v;
          break;
        case TimeSettingType::kHour:
          time_now->tm_hour += v;
          break;
        case TimeSettingType::kMin:
          time_now->tm_min += v;
          break;
      }
      timeval now{
          .tv_sec = mktime(time_now),
          .tv_usec = 0,
      };
      settimeofday(&now, nullptr);
    } break;
    default:
      break;
  }
}

static void update_time_ui() {
  tm* time_now;
  /// get time
  {
    time_t timer;
    time(&timer);
    time_now = localtime(&timer);
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
    static bool showPoint;
    static IntervalCall intervalCall(std::chrono::milliseconds(500), [] { showPoint = !showPoint; });
    intervalCall.poll();
    if (showPoint) {
      ledCanvas->drawPixel(15, 2, 1);
      ledCanvas->drawPixel(16, 2, 1);
      ledCanvas->drawPixel(15, 3, 1);
      ledCanvas->drawPixel(16, 3, 1);
      ledCanvas->drawPixel(15, 5, 1);
      ledCanvas->drawPixel(16, 5, 1);
      ledCanvas->drawPixel(15, 6, 1);
      ledCanvas->drawPixel(16, 6, 1);
    }
    // min
    sprintf(time_str, "%02d", time_now->tm_min);
    ledCanvas->setCursor(19, 1);
    ledCanvas->print(time_str);
  }
  /// bottom screen
  auto bottom_show_second = [&] {
    // small tv animation
    auto tv_data = time_now->tm_sec % 2 ? bili_tv_data1 : bili_tv_data2;
    ledCanvas->drawBitmap(0, 8, tv_data, bili_tv_width, bili_tv_height, 1);

    // diy animation
    static uint8_t array[]{};
    static IntervalCall intervalCall(std::chrono::milliseconds(300), [] {
      for (int i = 0; i < 9; ++i) {
        array[i] = esp_random() % 8;
      }
    });
    intervalCall.poll();
    for (int i = 0; i < 9; ++i) {
      ledCanvas->drawLine(9 + i, 16, 9 + i, 16 - array[i], 1);
    }

    // second display
    ledCanvas->setCursor(19, 9);
    char time_str[8];
    sprintf(time_str, "%02d", time_now->tm_sec);
    ledCanvas->print(time_str);
  };
  auto bottom_show_year = [&time_now] {
    char time_str[8];
    // 2020
    sprintf(time_str, "%d Y", 1900 + time_now->tm_year);
    ledCanvas->setCursor(2, 9);
    ledCanvas->print(time_str);
  };
  auto bottom_show_mon = [&time_now] {
    char time_str[8];
    // 02-18
    sprintf(time_str, "%02d-%02d", time_now->tm_mon, time_now->tm_mday);
    ledCanvas->setCursor(2, 9);
    ledCanvas->print(time_str);
  };

  auto show_user_set_type = [&] {
    switch (bottomShowType) {
      case BottomShowType::kSecond:
        bottom_show_second();
        break;
      case BottomShowType::kYear:
        bottom_show_year();
        break;
      case BottomShowType::kMon:
        bottom_show_mon();
        break;
    }
  };
  if (timeSettingType == TimeSettingType::kNone) {
    show_user_set_type();
  } else {
    // show settings
    static bool off = false;
    static IntervalCall intervalCall(std::chrono::milliseconds(300), [] { off = !off; });
    intervalCall.poll();
    switch (timeSettingType) {
      case TimeSettingType::kYear:
        if (!off) {
          bottom_show_year();
        }
        break;
      case TimeSettingType::kMon:
        bottom_show_mon();
        if (off) {
          ledCanvas->fillRect(0, 9, 14, 8, 0);
        }
        break;
      case TimeSettingType::kDay:
        bottom_show_mon();
        if (off) {
          ledCanvas->fillRect(19, 9, 13, 8, 0);
        }
        break;
      case TimeSettingType::kHour:
        show_user_set_type();
        if (off) {
          ledCanvas->fillRect(0, 0, 15, 8, 0);
        }
        break;
      case TimeSettingType::kMin:
        show_user_set_type();
        if (off) {
          ledCanvas->fillRect(17, 0, 14, 8, 0);
        }
        break;
      case TimeSettingType::kNone:
        break;
    }
  }

  ledCanvas->display();
}

extern "C" void app_main() {
  ESP_LOGI(TAG, "init");

  esp_init();
  sntp_env_init();
  start_wifi_task();
  config_button();

  ledMatrix = std::make_shared<LedMatrix>(GPIO_NUM_8, GPIO_NUM_6, GPIO_NUM_7, 8);
  for (int i = 0; i < 8; i++) {
    ledMatrix->shutdown(i, false);
    ledMatrix->setIntensity(i, 1);
    ledMatrix->clearDisplay(i);
  }

  ledCanvas = std::make_shared<LEDCanvas>(ledMatrix, 32, 16);
  show_loading();

  for (;;) {
    eventLoop.poll();
    check_button();
    update_time_ui();
    delay_ms(10);
  }
}
