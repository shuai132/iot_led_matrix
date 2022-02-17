#include "wifi_station.h"

#include <cstring>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

static const char* TAG = "wifi station";

static int s_retry_num_max;
static int s_retry_num;
static wifi_station_on_get_ip_t on_get_ip;
static std::function<void()> on_connect_failed;
static esp_event_handler_instance_t instance_any_id;
static esp_event_handler_instance_t instance_got_ip;
static EventGroupHandle_t s_wifi_event_group;

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
    esp_wifi_connect();
  } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
    if (s_retry_num < s_retry_num_max) {
      esp_wifi_connect();
      s_retry_num++;
      ESP_LOGI(TAG, "retry to connect to the AP");
    } else {
      xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
      if (on_connect_failed) on_connect_failed();
    }
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
    ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
    s_retry_num = 0;
    char ip[32];
    sprintf(ip, IPSTR, IP2STR(&event->ip_info.ip));
    if (on_get_ip) on_get_ip(ip);
    xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
  }
}

void wifi_station_init(const char* ssid, const char* passwd, wifi_station_on_get_ip_t on_get_ip_handle,
                       std::function<void()> on_connect_failed_handle, int retry_num_max) {
  on_get_ip = std::move(on_get_ip_handle);
  on_connect_failed = std::move(on_connect_failed_handle);
  s_retry_num_max = retry_num_max;

  s_wifi_event_group = xEventGroupCreate();

  ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, nullptr, &instance_any_id));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, nullptr, &instance_got_ip));

  wifi_config_t config{};
  strcpy((char*)config.sta.ssid, ssid);
  strcpy((char*)config.sta.password, passwd);
  config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
  config.sta.pmf_cfg = {.capable = true, .required = false};
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &config));
  ESP_ERROR_CHECK(esp_wifi_start());
}

bool wifi_station_wait_for_connect() {
  EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
  if (bits & WIFI_CONNECTED_BIT) {
    ESP_LOGI(TAG, "connected to ap");
    return true;
  } else if (bits & WIFI_FAIL_BIT) {
    ESP_LOGI(TAG, "Failed to connect");
  } else {
    ESP_LOGE(TAG, "UNEXPECTED EVENT");
  }
  return false;
}

void wifi_station_release() {
  /* The event will not be processed after unregister */
  ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
  ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
  vEventGroupDelete(s_wifi_event_group);
}
