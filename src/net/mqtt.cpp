#include "net/mqtt.h"

#include <cmath>
#include <cstdint>
#include <string>

#include <Arduino.h>
#include <ArduinoJson.h>

#include "config.h"
#include "core/identity.h"
#include "core/readings.h"
#include "core/settings.h"
#include "core/status_led.h"
#include "core/time.h"
#include "net/wifi.h"

namespace roomsense {
namespace {

// Topics below base_topic_.
constexpr const char* kStatusSuffix = "/status";
constexpr const char* kStateSuffix = "/state";

// Retained status payloads. "offline" is also the last will, so an
// unannounced disappearance is visible on the broker.
constexpr const char* kStatusOnline = "online";
constexpr const char* kStatusOffline = "offline";
constexpr uint8_t kStatusQos = 1;

// The state JSON is around 300 bytes, this leaves room for more fields.
constexpr uint16_t kBufferSize = 1024;

// Human readable form of PubSubClient::state().
const char* StateName(int state) {
  switch (state) {
    case MQTT_CONNECTION_TIMEOUT:
      return "connection timeout";
    case MQTT_CONNECTION_LOST:
      return "connection lost";
    case MQTT_CONNECT_FAILED:
      return "TCP connect failed";
    case MQTT_DISCONNECTED:
      return "disconnected";
    case MQTT_CONNECTED:
      return "connected";
    case MQTT_CONNECT_BAD_PROTOCOL:
      return "broker rejected protocol version";
    case MQTT_CONNECT_BAD_CLIENT_ID:
      return "broker rejected client id";
    case MQTT_CONNECT_UNAVAILABLE:
      return "broker unavailable";
    case MQTT_CONNECT_BAD_CREDENTIALS:
      return "bad credentials";
    case MQTT_CONNECT_UNAUTHORIZED:
      return "not authorized";
    default:
      return "unknown";
  }
}

}  // namespace

Mqtt& Mqtt::Instance() {
  // Intentionally never destroyed, see ReadingsStore::Instance().
  static Mqtt& instance = *new Mqtt();
  return instance;
}

void Mqtt::Tick() {
  const bool wifi_up = WifiIsConnected();
  // A dropped WiFi link leaves a dead socket behind that PubSubClient may
  // still report as connected, so a missing link counts as a disconnect.
  if (was_connected_ && (!wifi_up || !client_.connected())) {
    Serial.printf("MQTT: connection lost (%s)\n",
                  wifi_up ? StateName(client_.state()) : "WiFi down");
    client_.disconnect();
    SetNetworkState(NetState::kConnecting);
    was_connected_ = false;
  }
  if (!wifi_up) {
    return;
  }
  if (client_.connected()) {
    client_.loop();
    return;
  }
  if (!Reached(millis(), next_attempt_)) {
    return;
  }
  const Settings& settings = SettingsStore::Instance().Get();
  if (!Connect()) {
    next_attempt_ = millis() + kMqttRetryMs;
    Serial.printf("MQTT: connecting to %s:%u failed (%s), retrying in %u s\n",
                  settings.mqtt_host.c_str(), settings.mqtt_port, StateName(client_.state()),
                  kMqttRetryMs / 1000);
    return;
  }
  Serial.printf("MQTT: connected to %s:%u, topic %s\n", settings.mqtt_host.c_str(),
                settings.mqtt_port, base_topic_.c_str());
}

bool Mqtt::Connect() {
  const Settings& settings = SettingsStore::Instance().Get();
  base_topic_ = "roomsense/" + settings.device_name;
  const std::string status_topic = base_topic_ + kStatusSuffix;

  client_.setServer(settings.mqtt_host.c_str(), settings.mqtt_port);
  client_.setBufferSize(kBufferSize);

  // A null user connects anonymously.
  const char* user = settings.mqtt_user.empty() ? nullptr : settings.mqtt_user.c_str();
  const char* password = settings.mqtt_password.empty() ? nullptr : settings.mqtt_password.c_str();
  if (!client_.connect(Hostname().c_str(), user, password, status_topic.c_str(), kStatusQos,
                       /*willRetain=*/true, kStatusOffline)) {
    return false;
  }

  client_.publish(status_topic.c_str(), kStatusOnline, /*retained=*/true);
  SetNetworkState(NetState::kConnected);
  was_connected_ = true;
  return true;
}

bool Mqtt::IsConnected() { return client_.connected(); }

bool Mqtt::PublishState(const Readings& readings) {
  JsonDocument doc;
  auto put = [&doc](const char* key, float value) {
    if (std::isnan(value)) {
      doc[key] = nullptr;
    } else {
      doc[key] = roundf(value * 100.0f) / 100.0f;
    }
  };
  put("temp", readings.temperature);
  put("rh", readings.humidity);
  put("lux", readings.lux);
  put("pressure", readings.pressure);
  put("voc", readings.voc_index);
  put("nox", readings.nox_index);
  put("pm1", readings.pm_1);
  put("pm25", readings.pm_2_5);
  put("pm4", readings.pm_4);
  put("pm10", readings.pm_10);
  put("co2", readings.co2);
  put("noise_leq", readings.noise_leq);
  put("noise_max", readings.noise_max);
  doc["uptime"] = millis() / 1000;
  doc["rssi"] = WifiRssi();
  doc["fw"] = kFirmwareVersion;

  std::string payload;
  serializeJson(doc, payload);
  const std::string topic = base_topic_ + kStateSuffix;
  if (!client_.publish(topic.c_str(), payload.c_str())) {
    Serial.printf("MQTT: publishing %zu bytes to %s failed (%s)\n", payload.size(), topic.c_str(),
                  StateName(client_.state()));
    return false;
  }
  return true;
}

}  // namespace roomsense