#include "core/settings.h"

#include <string>

#include <Arduino.h>
#include <Preferences.h>

namespace roomsense {
namespace {

// NVS keys. Preferences limits them to 15 characters.
constexpr const char* kVersion = "version";
constexpr const char* kDeviceName = "device_name";
constexpr const char* kMqttHost = "mqtt_host";
constexpr const char* kMqttPort = "mqtt_port";
constexpr const char* kMqttUser = "mqtt_user";
constexpr const char* kMqttPassword = "mqtt_password";

std::string GetString(Preferences& prefs, const char* key, const std::string& fallback) {
  return prefs.getString(key, fallback.c_str()).c_str();
}

// False only if a non-empty value failed to write; an empty one writes no
// bytes and cannot be told apart from a failure.
bool PutString(Preferences& prefs, const char* key, const std::string& value) {
  const size_t written = prefs.putString(key, value.c_str());
  return written != 0 || value.empty();
}

// Upgrades `settings` from layout `from` to kSettingsVersion.
void Migrate(Settings& /*settings*/, uint16_t from) {
  // One case per old version, falling through so several steps chain.
  // Version 0 means keys written before versioning existed; their layout is
  // identical to version 1, so nothing to do yet.
  switch (from) {
    case 0:
      [[fallthrough]];
    default:
      break;
  }
}

}  // namespace

SettingsStore& SettingsStore::Instance() {
  static SettingsStore& store = *new SettingsStore();
  return store;
}

void SettingsStore::Load() {
  const Settings defaults;
  Preferences prefs;
  if (!prefs.begin(kNvsNamespace, /*readOnly=*/true)) {
    settings_ = defaults;
    loaded_version_ = 0;
    return;
  }
  loaded_version_ = prefs.getUShort(kVersion, 0);
  settings_.device_name = GetString(prefs, kDeviceName, defaults.device_name);
  settings_.mqtt_host = GetString(prefs, kMqttHost, defaults.mqtt_host);
  settings_.mqtt_port = prefs.getUShort(kMqttPort, defaults.mqtt_port);
  settings_.mqtt_user = GetString(prefs, kMqttUser, defaults.mqtt_user);
  settings_.mqtt_password = GetString(prefs, kMqttPassword, defaults.mqtt_password);
  prefs.end();

  if (loaded_version_ < kSettingsVersion) {
    Serial.printf("Settings: migrating layout %u -> %u\n", loaded_version_, kSettingsVersion);
    Migrate(settings_, loaded_version_);
    if (!Save(settings_)) {
      Serial.println("Settings: saving migrated settings failed");
    }
  } else if (loaded_version_ > kSettingsVersion) {
    Serial.printf("Settings: layout %u is newer than this firmware (%u)\n", loaded_version_,
                  kSettingsVersion);
  }
}

bool SettingsStore::Save(const Settings& settings) {
  Preferences prefs;
  if (!prefs.begin(kNvsNamespace, /*readOnly=*/false)) {
    Serial.println("Settings: opening NVS for writing failed");
    return false;
  }
  // put* return the number of bytes written, 0 on failure. An empty string
  // legitimately writes 0 bytes, so only non-empty ones can be checked.
  bool written = prefs.putUShort(kVersion, kSettingsVersion) != 0;
  written = prefs.putUShort(kMqttPort, settings.mqtt_port) != 0 && written;
  written = PutString(prefs, kDeviceName, settings.device_name) && written;
  written = PutString(prefs, kMqttHost, settings.mqtt_host) && written;
  written = PutString(prefs, kMqttUser, settings.mqtt_user) && written;
  written = PutString(prefs, kMqttPassword, settings.mqtt_password) && written;
  prefs.end();
  if (!written) {
    Serial.println("Settings: writing to NVS failed");
    return false;
  }
  settings_ = settings;
  return true;
}

bool SettingsStore::Clear() {
  Preferences prefs;
  if (!prefs.begin(kNvsNamespace, /*readOnly=*/false)) {
    Serial.println("Settings: opening NVS for writing failed");
    return false;
  }
  const bool written = prefs.clear();
  prefs.end();
  if (!written) {
    Serial.println("Settings: clearing NVS failed");
    return false;
  }
  settings_ = Settings();
  return true;
}

bool SettingsStore::IsConfigured() const {
  return !settings_.device_name.empty() && !settings_.mqtt_host.empty();
}

}  // namespace roomsense
