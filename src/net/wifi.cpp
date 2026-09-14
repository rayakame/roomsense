#include "net/wifi.h"

#include <cstdint>

#include <Arduino.h>
#include <WiFi.h>

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
uint32_t next_attempt = 0;

}  // namespace
void WifiTick() {
  if (!started) {
    WiFiClass::mode(WIFI_STA);
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
      // Only Mqtt::Connect() reports kConnected; a link without a broker is
      // not a working device.
      SetNetworkState(NetState::kConnecting);
    }
    return;
  }
  if (connected) {
    Serial.println("WiFi: connection lost");
    SetNetworkState(NetState::kDisconnected);
    connected = false;
  }
  if (!Reached(millis(), next_attempt)) {
    return;
  }
  Serial.printf("WiFi: connecting to %s\n", WiFi.SSID().c_str());
  WiFi.begin();
  next_attempt = millis() + kWifiRetryMs;
}

int WifiRssi() {
  if (!connected) {
    return 0;
  }
  return WiFi.RSSI();
}

bool WifiIsConnected() { return connected; }

}  // namespace roomsense
