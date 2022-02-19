#pragma once

#include <chrono>
#include <functional>

class IntervalCall {
 public:
  explicit IntervalCall(std::chrono::steady_clock::duration interval, std::function<void()> cb = nullptr) : interval_(interval), cb_(std::move(cb)) {}

  bool poll() {
    auto now = std::chrono::steady_clock::now();
    if (now - lastTime_ > interval_) {
      lastTime_ = now;
      if (cb_) cb_();
      return true;
    }
    return false;
  }

 private:
  std::chrono::steady_clock::duration interval_;
  std::function<void()> cb_;
  std::chrono::steady_clock::time_point lastTime_;
};
