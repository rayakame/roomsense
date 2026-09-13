#pragma once

#include <cstdint>
#include <string>

#include <PubSubClient.h>
#include <WiFiClient.h>

#include "core/readings.h"

namespace roomsense {

class Mqtt {
 public:
  static Mqtt& Instance();
  // Reconnects when needed and services the client. Call once per net tick.
  void Tick();
  bool IsConnected();
  bool PublishState(const Readings& readings);

 private:
  bool Connect();

  WiFiClient wifi_client_;
  PubSubClient client_{wifi_client_};
  // True while the last Connect() is still considered live, so Tick() can
  // report the transition to disconnected exactly once.
  bool was_connected_ = false;
  uint32_t next_attempt_ = 0;
  std::string base_topic_;
};

}  // namespace roomsense
