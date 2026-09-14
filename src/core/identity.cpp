#include "core/identity.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>

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

std::string ReadBuildTimestamp() {
  // __DATE__ is "Sep 14 2026", with a leading space instead of a zero for
  // single digit days. __TIME__ is "10:22:31".
  constexpr std::string_view kMonths = "JanFebMarAprMayJunJulAugSepOctNovDec";
  constexpr std::string_view kDate = __DATE__;
  const std::size_t month = (kMonths.find(kDate.substr(0, 3)) / 3) + 1;
  const char day_tens = kDate[4] == ' ' ? '0' : kDate[4];
  std::array<char, 20> text = {};
  snprintf(text.data(), text.size(), "%.4s-%02u-%c%c %s", kDate.data() + 7,
           static_cast<unsigned>(month), day_tens, kDate[5], __TIME__);
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

const std::string& BuildTimestamp() {
  static const std::string& timestamp = *new std::string(ReadBuildTimestamp());
  return timestamp;
}

}  // namespace roomsense
