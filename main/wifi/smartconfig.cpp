#include "smartconfig.h"

#include <cstdlib>
#include <cstring>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_smartconfig.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_wpa2.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

static WiFiInfoHandle update_cb DRAM_ATTR;       // 当前工具链可能有问题，需添加`DRAM_ATTR`，否则赋值时崩溃，原因未知
static ConnectStateHandle connect_cb DRAM_ATTR;  // 同上

/* FreeRTOS event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
static const int CONNECTED_BIT = BIT0;
static const int ESPTOUCH_DONE_BIT = BIT1;
static const char* TAG = "smartconfig";

static void smartconfig_task(void* parm);

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
    xTaskCreate(smartconfig_task, "smartconfig_task", 4096, nullptr, 3, nullptr);
  } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
    esp_wifi_connect();
    xEventGroupClearBits(s_wifi_event_group, CONNECTED_BIT);
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    xEventGroupSetBits(s_wifi_event_group, CONNECTED_BIT);
    ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
    ESP_LOGD(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
    char ip[32];
    sprintf(ip, IPSTR, IP2STR(&event->ip_info.ip));
    connect_cb(ConnectState::Connected, ip);
  } else if (event_base == SC_EVENT && event_id == SC_EVENT_SCAN_DONE) {
    ESP_LOGI(TAG, "Scan done");
  } else if (event_base == SC_EVENT && event_id == SC_EVENT_FOUND_CHANNEL) {
    ESP_LOGI(TAG, "Found channel");
  } else if (event_base == SC_EVENT && event_id == SC_EVENT_GOT_SSID_PSWD) {
    ESP_LOGI(TAG, "Got SSID and password");

    auto* evt = (smartconfig_event_got_ssid_pswd_t*)event_data;
    update_cb({(char*)evt->ssid, (char*)evt->password});

    wifi_config_t wifi_config;
    uint8_t ssid[33] = {0};
    uint8_t password[65] = {0};
    uint8_t rvd_data[33] = {0};

    bzero(&wifi_config, sizeof(wifi_config_t));
    memcpy(wifi_config.sta.ssid, evt->ssid, sizeof(wifi_config.sta.ssid));
    memcpy(wifi_config.sta.password, evt->password, sizeof(wifi_config.sta.password));
    wifi_config.sta.bssid_set = evt->bssid_set;
    if (wifi_config.sta.bssid_set) {
      memcpy(wifi_config.sta.bssid, evt->bssid, sizeof(wifi_config.sta.bssid));
    }

    memcpy(ssid, evt->ssid, sizeof(evt->ssid));
    memcpy(password, evt->password, sizeof(evt->password));
    ESP_LOGI(TAG, "SSID:%s", ssid);
    ESP_LOGI(TAG, "PASSWORD:%s", password);
    if (evt->type == SC_TYPE_ESPTOUCH_V2) {
      ESP_ERROR_CHECK(esp_smartconfig_get_rvd_data(rvd_data, sizeof(rvd_data)));
      ESP_LOGI(TAG, "RVD_DATA:");
      for (unsigned char i : rvd_data) {
        printf("%02x ", i);
      }
      printf("\n");
    }

    ESP_ERROR_CHECK(esp_wifi_disconnect());
    connect_cb(ConnectState::Disconnected, nullptr);
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    esp_wifi_connect();
    connect_cb(ConnectState::Connecting, nullptr);
  } else if (event_base == SC_EVENT && event_id == SC_EVENT_SEND_ACK_DONE) {
    xEventGroupSetBits(s_wifi_event_group, ESPTOUCH_DONE_BIT);
  }
}

static void smartconfig_task(void* parm) {
  ESP_ERROR_CHECK(esp_smartconfig_set_type(SC_TYPE_ESPTOUCH_AIRKISS));
  smartconfig_start_config_t cfg = SMARTCONFIG_START_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_smartconfig_start(&cfg));
  while (true) {
    xEventGroupWaitBits(s_wifi_event_group, ESPTOUCH_DONE_BIT, true, false, portMAX_DELAY);
    ESP_LOGI(TAG, "smartconfig over");
    esp_smartconfig_stop();
    vTaskDelete(nullptr);
  }
}

void start_smartconfig_task(WiFiInfoHandle updateCb, ConnectStateHandle connectCb) {
  update_cb = std::move(updateCb);
  connect_cb = std::move(connectCb);

  s_wifi_event_group = xEventGroupCreate();

  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, nullptr));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, nullptr));
  ESP_ERROR_CHECK(esp_event_handler_register(SC_EVENT, ESP_EVENT_ANY_ID, &event_handler, nullptr));

  ESP_ERROR_CHECK(esp_wifi_stop());
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_start());
}
