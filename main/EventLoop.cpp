#include "EventLoop.h"

void EventLoop::post(Runnable runnable) {
  std::lock_guard<std::mutex> lock(mutex_lock_);
  event_queue_.push(std::move(runnable));
}

void EventLoop::poll() {
  std::lock_guard<std::mutex> lock(mutex_lock_);
  while (!event_queue_.empty()) {
    const auto& runnable = event_queue_.front();
    if (runnable) {
      runnable();
      event_queue_.pop();
    }
  }
}
