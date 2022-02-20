#include "adc_dma.h"

#include <cstring>

#include "driver/adc.h"
#include "esp_log.h"
#include "sdkconfig.h"

#define GET_UNIT(x) ((x >> 3) & 0x1)

static const char* TAG = "ADC_DMA";

ADC::~ADC() {
  ESP_ERROR_CHECK(adc_digi_stop());
  ESP_ERROR_CHECK(adc_digi_deinitialize());
}

void ADC::start(uint32_t fs, uint32_t sn) {
  fs_ = fs;
  sn_ = sn;
  adc_channel_t channel[] = {static_cast<adc_channel_t>(ADC1_CHANNEL_0)};
  uint8_t channel_num = sizeof(channel) / sizeof(adc_channel_t);
  adc_digi_init_config_t adc_dma_config = {
      .max_store_buf_size = sn * sizeof(adc_digi_output_data_t),
      .conv_num_each_intr = sn,
      .adc1_chan_mask = BIT(0),
      .adc2_chan_mask = 0,
  };
  ESP_ERROR_CHECK(adc_digi_initialize(&adc_dma_config));

  adc_digi_configuration_t dig_cfg = {
      .conv_limit_en = false,
      .conv_limit_num = 0,
      .sample_freq_hz = fs,
      .conv_mode = ADC_CONV_ALTER_UNIT,
      .format = ADC_DIGI_OUTPUT_FORMAT_TYPE2,
  };

  adc_digi_pattern_config_t adc_pattern[SOC_ADC_PATT_LEN_MAX] = {{0}};
  dig_cfg.pattern_num = channel_num;
  for (int i = 0; i < channel_num; i++) {
    uint8_t unit = GET_UNIT(channel[i]);
    uint8_t ch = channel[i] & 0x7;
    adc_pattern[i].atten = ADC_ATTEN_DB_11;
    adc_pattern[i].channel = ch;
    adc_pattern[i].unit = unit;
    adc_pattern[i].bit_width = SOC_ADC_DIGI_MAX_BITWIDTH;

    ESP_LOGI(TAG, "adc_pattern[%d].atten is :%x", i, adc_pattern[i].atten);
    ESP_LOGI(TAG, "adc_pattern[%d].channel is :%x", i, adc_pattern[i].channel);
    ESP_LOGI(TAG, "adc_pattern[%d].unit is :%x", i, adc_pattern[i].unit);
  }
  dig_cfg.adc_pattern = adc_pattern;
  ESP_ERROR_CHECK(adc_digi_controller_configure(&dig_cfg));

  buffer_.resize(sn);
  adc_digi_start();
}

const std::vector<uint16_t>& ADC::readData() {
  std::vector<adc_digi_output_data_t> data;
  data.resize(buffer_.size());
  auto ret =
      adc_digi_read_bytes(reinterpret_cast<uint8_t*>(data.data()), data.size() * sizeof(adc_digi_output_data_t), &availableSize_, ADC_MAX_DELAY);
  if (ret == ESP_OK || ret == ESP_ERR_INVALID_STATE) {
    // normal!
  }
  for (int i = 0; i < buffer_.size(); ++i) {
    buffer_[i] = data[i].type2.data;
  }
  return buffer_;
}

uint32_t ADC::getAvailableSize() const { return availableSize_; }
uint32_t ADC::getFs() const { return fs_; }
uint32_t ADC::getSn() const { return sn_; }
