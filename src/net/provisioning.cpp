#include "net/provisioning.h"

#include <algorithm>
#include <string>
#include <vector>

#include <Arduino.h>
#include <WiFiManager.h>

#include "core/identity.h"
#include "core/settings.h"

namespace roomsense {
namespace {

constexpr int kBootButtonPin = 0;
constexpr int kBootHoldMs = 3000;
constexpr int kPortalTimeoutS = 600;
constexpr int kConnectTimeoutS = 20;

constexpr const char* kValidationScript = R"html(
<script>
document.addEventListener('DOMContentLoaded', function () {
  var form = document.querySelector("form[action='paramsave']");
  if (!form) return;
  form.addEventListener('submit', function (event) {
    var name = document.getElementById('name');
    var host = document.getElementById('host');
    var port = document.getElementById('port');
    name.value = name.value.trim().toLowerCase();
    host.value = host.value.trim();
    var error = '';
    if (!/^[a-z0-9-]{1,32}$/.test(name.value)) {
      error = 'Device name: 1-32 characters, only a-z, 0-9 and -';
    } else if (!host.value) {
      error = 'MQTT host is required';
    } else if (!(port.value >= 1 && port.value <= 65535)) {
      error = 'MQTT port must be between 1 and 65535';
    }
    if (error) {
      event.preventDefault();
      alert(error);
    }
  });
});
</script>
)html";

bool BootButtonHeld() {
  pinMode(kBootButtonPin, INPUT_PULLUP);
  for (uint32_t held = 0; held < kBootHoldMs; held += 100) {
    if (digitalRead(kBootButtonPin) == HIGH) {
      return false;
    }
    delay(100);
  }
  return true;
}

bool IsValidDeviceName(const std::string& name) {
  if (name.empty() || name.size() > 32) {
    return false;
  }
  return std::ranges::all_of(name, [](const char character) {
    return (character >= 'a' && character <= 'z') || (character >= '0' && character <= '9') ||
           character == '-';
  });
}

bool SaveFromForm(const WiFiManagerParameter& name, const WiFiManagerParameter& host,
                  const WiFiManagerParameter& port, const WiFiManagerParameter& user,
                  const WiFiManagerParameter& pass) {
  Settings settings;
  settings.device_name = name.getValue();
  settings.mqtt_host = host.getValue();
  const uint32_t mqtt_port = strtoul(port.getValue(), nullptr, 10);
  settings.mqtt_user = user.getValue();
  settings.mqtt_password = pass.getValue();
  if (!IsValidDeviceName(settings.device_name) || settings.mqtt_host.empty() || mqtt_port < 1 ||
      mqtt_port > 65535) {
    Serial.printf("Provisioning: rejected form data name='%s' host='%s' port='%s'\n",
                  name.getValue(), host.getValue(), port.getValue());
    return false;
  }
  settings.mqtt_port = static_cast<uint16_t>(mqtt_port);
  return SettingsStore::Instance().Save(settings);
}

}  // namespace

void RunProvisioning() {
  WiFiManager manager;
  manager.setHostname(Hostname().c_str());
  manager.setTitle("roomsense");
  manager.setConfigPortalTimeout(kPortalTimeoutS);
  manager.setConnectTimeout(kConnectTimeoutS);
  manager.setCustomHeadElement(kValidationScript);
  std::vector<const char*> menu = {"wifi", "param", "info", "exit"};
  manager.setMenu(menu);

  if (BootButtonHeld()) {
    Serial.println("Provisioning: factory reset");
    manager.resetSettings();
    SettingsStore::Instance().Clear();
  }

  const Settings& cur = SettingsStore::Instance().Get();
  const std::string port_str = std::to_string(cur.mqtt_port);
  WiFiManagerParameter name(
      "name", "Device name (a-z, 0-9, -)", cur.device_name.c_str(), 32,
      " required pattern='[a-z0-9-]{1,32}' autocapitalize='none' autocorrect='off'"
      " title='1-32 characters, only lowercase letters, digits and hyphen. Used as MQTT topic"
      " prefix.'");
  WiFiManagerParameter host("host", "MQTT host", cur.mqtt_host.c_str(), 64,
                            " required title='Hostname or IP address of the MQTT broker'");
  WiFiManagerParameter port("port", "MQTT port", port_str.c_str(), 5,
                            " type='number' min='1' max='65535'"
                            " title='TCP port of the MQTT broker, usually 1883'");
  WiFiManagerParameter user("user", "MQTT user (optional)", cur.mqtt_user.c_str(), 32,
                            " title='Leave empty if the broker allows anonymous access'");
  WiFiManagerParameter pass("pass", "MQTT password (optional)", cur.mqtt_password.c_str(), 64,
                            " type='password' title='Leave empty if the broker allows anonymous"
                            " access'");
  manager.addParameter(&name);
  manager.addParameter(&host);
  manager.addParameter(&port);
  manager.addParameter(&user);
  manager.addParameter(&pass);
  manager.setSaveParamsCallback([&] { SaveFromForm(name, host, port, user, pass); });

  const bool need_portal = !manager.getWiFiIsSaved() || !SettingsStore::Instance().IsConfigured();
  if (need_portal) {
    Serial.printf("Provisioning: starting portal %s\n", Hostname().c_str());
    if (!manager.startConfigPortal(Hostname().c_str())) {
      Serial.println("Provisioning: portal timed out, restarting");
      ESP.restart();
    }
  } else {
    manager.setEnableConfigPortal(false);
    if (!manager.autoConnect(Hostname().c_str())) {
      Serial.println("Provisioning: WiFi not reachable, continuing offline");
    }
  }

  if (!SettingsStore::Instance().IsConfigured()) {
    Serial.println("Provisioning: settings missing after portal, restarting");
    ESP.restart();
  }
}

}  // namespace roomsense
