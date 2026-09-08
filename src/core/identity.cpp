#include "core/identity.h"

#include <array>
#include <cstdint>
#include <cstdio>
#include <string>

#include <esp_mac.h>

namespace roomsense {
namespace {

std::string ReadDeviceId() {
  std::array<uint8_t, 6> mac = {};
  esp_efuse_mac_get_default(mac.data());
  std::array<char, 7> text = {};
  snprintf(text.data(), text.size(), "%02x%02x%02x", mac[3], mac[4], mac[5]);
  return {text.data()};
}

}  // namespace

const std::string& DeviceId() {
  // Intentionally never destroyed, see ReadingsStore::Instance().
  static const std::string& device_id = *new std::string(ReadDeviceId());
  return device_id;
}

const std::string& Hostname() {
  static const std::string& hostname = *new std::string("roomsense-" + DeviceId());
  return hostname;
}

}  // namespace roomsense
