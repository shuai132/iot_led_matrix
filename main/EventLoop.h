#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>

class EventLoop {
  using Runnable = std::function<void()>;

 public:
  EventLoop() = default;
  EventLoop(const EventLoop&) = delete;
  const EventLoop& operator=(const EventLoop&) = delete;

  void post(Runnable runnable);

  void poll();

 private:
  std::queue<Runnable> event_queue_;
  std::mutex mutex_lock_;
};
