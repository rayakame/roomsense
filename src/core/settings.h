#pragma once

#include <cstdint>
#include <string>

namespace roomsense {

// Layout version of the stored settings. Bump it whenever a field is added,
// removed or changes meaning, and add a step to Migrate() in settings.cpp.
constexpr uint16_t kSettingsVersion = 1;

struct Settings {
  std::string device_name;  // required, [a-z0-9-]{1,32}
  std::string mqtt_host;    // required
  uint16_t mqtt_port = 1883;
  std::string mqtt_user;      // optional
  std::string mqtt_password;  // optional
};

class SettingsStore {
 public:
  static SettingsStore& Instance();

  void Load();
  // Stores `settings` and makes them the current ones. False if NVS failed,
  // the previous settings then stay current.
  bool Save(const Settings& settings);
  // Deletes everything stored and resets to defaults. False if NVS failed.
  bool Clear();

  const Settings& Get() const { return settings_; }
  bool IsConfigured() const;
  // Layout version found in NVS by Load(), 0 if nothing was stored.
  uint16_t loaded_version() const { return loaded_version_; }

 private:
  Settings settings_;
  uint16_t loaded_version_ = 0;
};

}  // namespace roomsense
