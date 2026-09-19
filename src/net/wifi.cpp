#include "net/wifi.h"

#include <cstdint>
#include <cstring>
#include <string>

#include <Arduino.h>
#include <WiFi.h>
#include <esp_err.h>
#include <esp_wifi.h>

#include "config.h"
#include "core/diagnostics.h"
#include "core/identity.h"
#include "core/status_led.h"
#include "core/time.h"

namespace roomsense {
namespace {

bool connected = false;
bool ever_connected = false;
bool started = false;
Deadline next_attempt;

// The network the driver will dial. WiFi.SSID() only knows the one we are
// associated with, so it is empty exactly when we want to print it. Both
// markers below mean the driver accepted a connect for a config it does not
// have, so neither should ever reach the log.
std::string ConfiguredSsid() {
  wifi_config_t config = {};
  if (esp_wifi_get_config(WIFI_IF_STA, &config) != ESP_OK) {
    return "<unreadable>";
  }
  // Not necessarily terminated: 32 characters fill the field exactly.
  const char* ssid = reinterpret_cast<const char*>(config.sta.ssid);
  const size_t length = strnlen(ssid, sizeof(config.sta.ssid));
  if (length == 0) {
    return "<none>";
  }
  return {ssid, length};
}

// One attempt. Not WiFi.begin(): that writes the driver config first, which
// the driver rejects while an attempt is already running.
void StartAttempt() {
  if (!WiFi.STA.started()) {
    // Switched off, e.g. by a factory reset. Connect on the next tick.
    WiFi.STA.begin(/*tryConnect=*/false);
    return;
  }
  if (WiFi.STA.connected()) {
    // Associated, only the IP is missing. Do not abort a running DHCP.
    return;
  }

  const esp_err_t err = esp_wifi_connect();
  switch (err) {
    case ESP_OK:
      Serial.printf("WiFi: connecting to %s\n", ConfiguredSsid().c_str());
      break;
    case ESP_ERR_WIFI_CONN:
      // An attempt is already running, normally the framework's own retry.
      break;
    case ESP_ERR_WIFI_SSID:
      Serial.println("WiFi: no credentials stored");
      break;
    default:
      Serial.printf("WiFi: connect failed (%s)\n", esp_err_to_name(err));
      break;
  }
}

}  // namespace

void WifiTick() {
  if (!started) {
    WiFiClass::mode(WIFI_STA);
    // The framework does the reconnecting. The loop below only covers the
    // disconnect reasons it skips, above all a wrong password.
    WiFi.setAutoReconnect(true);
    WiFiClass::setHostname(Hostname().c_str());
    started = true;
  }
  if (WiFi.status() == WL_CONNECTED) {
    if (!connected) {
      Serial.printf("WiFi: connected to %s, IP %s, RSSI %d dBm\n", WiFi.SSID().c_str(),
                    WiFi.localIP().toString().c_str(), WiFi.RSSI());
      connected = true;
      if (ever_connected) {
        CountWifiReconnect();
      }
      ever_connected = true;
      // A link without a broker is not a working device, so only
      // Mqtt::Connect() reports kConnected.
      SetNetworkState(NetState::kConnecting);
    }
    return;
  }
  if (connected) {
    Serial.println("WiFi: connection lost");
    SetNetworkState(NetState::kDisconnected);
    connected = false;
  }
  if (!next_attempt.Elapsed()) {
    return;
  }
  next_attempt.ArmIn(kWifiRetryMs);
  StartAttempt();
}

int WifiRssi() {
  if (!connected) {
    return 0;
  }
  return WiFi.RSSI();
}

bool WifiIsConnected() { return connected; }

}  // namespace roomsense
