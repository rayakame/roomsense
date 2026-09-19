#include "net/mqtt.h"

#include <cmath>
#include <cstdint>
#include <string>

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>

#include "config.h"
#include "core/diagnostics.h"
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
constexpr const char* kInfoSuffix = "/info";
constexpr const char* kDiagSuffix = "/diag";

// Retained status payloads. "offline" is also the last will, so an
// unannounced disappearance is visible on the broker.
constexpr const char* kStatusOnline = "online";
constexpr const char* kStatusOffline = "offline";
constexpr uint8_t kStatusQos = 1;

// The largest message is around 400 bytes, this leaves room for more fields.
constexpr uint16_t kBufferSize = 1024;

constexpr uint32_t kBytesPerMb = 1024 * 1024;

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
  // PubSubClient may still call a socket alive after the link dropped, so a
  // missing link counts as a disconnect.
  if (was_connected_ && (!wifi_up || !client_.connected())) {
    Serial.printf("MQTT: connection lost (%s)\n",
                  wifi_up ? StateName(client_.state()) : "WiFi down");
    client_.disconnect();
    if (wifi_up) {
      // Only the broker is missing. With the link down WifiTick() already
      // reported kDisconnected, and overwriting that would hide the outage.
      SetNetworkState(NetState::kConnecting);
    }
    was_connected_ = false;
  }
  if (!wifi_up) {
    return;
  }
  if (client_.connected()) {
    client_.loop();
    return;
  }
  if (!next_attempt_.Elapsed()) {
    return;
  }
  const Settings& settings = SettingsStore::Instance().Get();
  if (!Connect()) {
    next_attempt_.ArmIn(kMqttRetryMs);
    Serial.printf("MQTT: connecting to %s:%u failed (%s), retrying in %u s\n",
                  settings.mqtt_host.c_str(), settings.mqtt_port, StateName(client_.state()),
                  static_cast<unsigned>(kMqttRetryMs / 1000));
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
  if (!client_.setBufferSize(kBufferSize)) {
    // Every publish would exceed the 256 byte default, so do not connect.
    Serial.println("MQTT: no memory for the send buffer");
    return false;
  }

  // A null user connects anonymously.
  const char* user = settings.mqtt_user.empty() ? nullptr : settings.mqtt_user.c_str();
  const char* password = settings.mqtt_password.empty() ? nullptr : settings.mqtt_password.c_str();
  if (!client_.connect(Hostname().c_str(), user, password, status_topic.c_str(), kStatusQos,
                       /*willRetain=*/true, kStatusOffline)) {
    return false;
  }

  Publish(status_topic, kStatusOnline, /*retained=*/true);
  SetNetworkState(NetState::kConnected);
  was_connected_ = true;
  if (ever_connected_) {
    CountMqttReconnect();
  }
  ever_connected_ = true;
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
  return Publish(base_topic_ + kStateSuffix, payload, /*retained=*/false);
}

bool Mqtt::PublishInfo() {
  JsonDocument doc;
  // A constant, so the message works as an info metric. Everything that
  // matters sits in the string fields.
  doc["info"] = 1;
  doc["fw"] = kFirmwareVersion;
  doc["built"] = BuildTimestamp();
  doc["id"] = DeviceId();
  doc["chip"] = ESP.getChipModel();
  // The core reports the revision as major * 100 + minor.
  const uint16_t revision = ESP.getChipRevision();
  doc["rev"] = std::to_string(revision / 100) + "." + std::to_string(revision % 100);
  doc["cpu_mhz"] = std::to_string(ESP.getCpuFreqMHz());
  doc["flash_mb"] = std::to_string(ESP.getFlashChipSize() / kBytesPerMb);
  doc["sdk"] = ESP.getSdkVersion();
  doc["reset_reason"] = ResetReasonName();
  // toString() returns a temporary, so copy before ArduinoJson sees it.
  doc["ip"] = std::string(WiFi.localIP().toString().c_str());
  doc["bssid"] = std::string(WiFi.BSSIDstr().c_str());

  std::string payload;
  serializeJson(doc, payload);
  return Publish(base_topic_ + kInfoSuffix, payload, /*retained=*/true);
}

bool Mqtt::PublishDiag() {
  JsonDocument doc;
  doc["heap_free"] = ESP.getFreeHeap();
  doc["heap_min"] = ESP.getMinFreeHeap();
  doc["heap_largest"] = ESP.getMaxAllocHeap();
  doc["cpu_temp"] = roundf(temperatureRead() * 10.0f) / 10.0f;
  doc["stack_sensor"] = TaskStackFreeBytes(TaskId::kSensor);
  doc["stack_mic"] = TaskStackFreeBytes(TaskId::kMic);
  doc["stack_net"] = TaskStackFreeBytes(TaskId::kNet);
  doc["wifi_reconnects"] = WifiReconnects();
  doc["mqtt_reconnects"] = MqttReconnects();
  doc["publish_failures"] = PublishFailures();
  doc["sensor_retries"] = SensorRetries();
  doc["wifi_channel"] = WiFi.channel();

  // Without these, a null in the state message could mean broken, missing or
  // still warming up.
  const Readings readings = ReadingsStore::Instance().Snapshot();
  doc["ok_sht"] = readings.sht_ok ? 1 : 0;
  doc["ok_veml"] = readings.veml_ok ? 1 : 0;
  doc["ok_dps"] = readings.dps_ok ? 1 : 0;
  doc["ok_sgp"] = readings.sgp_ok ? 1 : 0;
  doc["ok_sps"] = readings.sps_ok ? 1 : 0;
  doc["ok_scd"] = readings.scd_ok ? 1 : 0;
  doc["ok_mic"] = readings.mic_ok ? 1 : 0;

  std::string payload;
  serializeJson(doc, payload);
  return Publish(base_topic_ + kDiagSuffix, payload, /*retained=*/false);
}

bool Mqtt::Publish(const std::string& topic, const std::string& payload, bool retained) {
  if (client_.publish(topic.c_str(), payload.c_str(), retained)) {
    return true;
  }
  CountPublishFailure();
  Serial.printf("MQTT: publishing %zu bytes to %s failed (%s)\n", payload.size(), topic.c_str(),
                StateName(client_.state()));
  return false;
}

}  // namespace roomsense
