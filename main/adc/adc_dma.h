#pragma once

#include <cstdint>
#include <vector>

class ADC {
 public:
  ADC() = default;
  ADC(const ADC&) = delete;
  const ADC& operator=(const ADC&) = delete;
  ~ADC();

 public:
  void start(uint32_t fs, uint32_t sn);

  const std::vector<uint16_t>& readData();
  uint32_t getAvailableSize() const;
  uint32_t getFs() const;
  uint32_t getSn() const;

 private:
  std::vector<uint16_t> buffer_;
  uint32_t availableSize_ = 0;
  uint32_t fs_ = 0;
  uint32_t sn_ = 0;
};
