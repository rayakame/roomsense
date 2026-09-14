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
  // Sensor values, published on every state interval.
  bool PublishState(const Readings& readings);
  // Identity as strings: chip, firmware, IP, reset reason. Retained, so a
  // subscriber sees it right after connecting.
  bool PublishInfo();
  // Runtime health as numbers: heap, temperature, stack, counters. Not
  // retained, these are only interesting as a time series.
  bool PublishDiag();

 private:
  bool Connect();
  bool Publish(const std::string& topic, const std::string& payload, bool retained);

  WiFiClient wifi_client_;
  PubSubClient client_{wifi_client_};
  // True while the last Connect() is still considered live, so Tick() can
  // report the transition to disconnected exactly once.
  bool was_connected_ = false;
  // True once the first connection succeeded, so Connect() can tell a
  // reconnect from the initial one.
  bool ever_connected_ = false;
  uint32_t next_attempt_ = 0;
  std::string base_topic_;
};

}  // namespace roomsense
