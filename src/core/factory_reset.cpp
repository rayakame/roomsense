#include "core/factory_reset.h"

#include <atomic>
#include <cstdint>

#include <Arduino.h>
#include <WiFi.h>

#include "core/settings.h"
#include "core/status_led.h"

namespace roomsense {
namespace {

// Erasing the credentials is refused while the driver has a connection attempt
// running, and there is no way to ask whether it has one. Retry a few times.
constexpr int kEraseAttempts = 5;
constexpr uint32_t kEraseRetryMs = 100;

// Long enough for the red confirmation and for the log to leave the buffer.
constexpr uint32_t kConfirmMs = 1000;

std::atomic<bool> requested = false;

}  // namespace

void RequestFactoryReset() { requested = true; }

void RunFactoryResetIfRequested() {
  if (!requested.load()) {
    return;
  }
  Serial.println("Factory reset: wiping settings and credentials");
  ShowFactoryReset();

  // Settings first: with them gone the device opens the setup portal on the
  // next boot even if erasing the credentials below fails.
  if (!SettingsStore::Instance().Clear()) {
    Serial.println("Factory reset: clearing the settings failed");
  }

  // Stop the framework from starting another attempt between our tries.
  WiFi.setAutoReconnect(false);
  bool erased = false;
  for (int attempt = 0; attempt < kEraseAttempts && !erased; attempt++) {
    erased = WiFi.disconnect(/*wifioff=*/true, /*eraseap=*/true);
    if (!erased) {
      delay(kEraseRetryMs);
    }
  }
  if (!erased) {
    Serial.println("Factory reset: erasing the WiFi credentials failed");
  }

  delay(kConfirmMs);
  ESP.restart();
}

}  // namespace roomsense
