#pragma once

#include <functional>
#include <string>

struct WiFiInfo {
  std::string ssid;
  std::string passwd;
};

enum class ConnectState {
  Disconnected,
  Connecting,
  Connected,
};

using WiFiInfoHandle = std::function<void(WiFiInfo)>;
using ConnectStateHandle = std::function<void(ConnectState, void* /* ip or null */)>;

void start_smartconfig_task(WiFiInfoHandle updateCb, ConnectStateHandle connectCb);
