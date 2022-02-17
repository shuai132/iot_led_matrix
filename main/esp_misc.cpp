#include "esp_misc.h"

#include <esp_event.h>
#include <esp_wifi.h>
#include <esp_wifi_default.h>
#include <nvs_flash.h>

static void wifi_init() {
  ESP_ERROR_CHECK(esp_netif_init());
  assert(esp_netif_create_default_wifi_sta());
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
}

void esp_init() {
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  ESP_ERROR_CHECK(esp_event_loop_create_default());

  wifi_init();
}

void delay_ms(uint32_t ms) { vTaskDelay(ms / portTICK_PERIOD_MS); }
