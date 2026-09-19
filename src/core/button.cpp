#include "core/button.h"

#include <cstdint>

#include <Arduino.h>
#include <WiFi.h>

#include "config.h"
#include "core/settings.h"
#include "core/status_led.h"

namespace roomsense {
namespace {

// Never initialized at startup, so it survives a restart but not a power
// loss. The magic value tells a real request from leftover noise.
RTC_NOINIT_ATTR uint32_t portal_request;
constexpr uint32_t kPortalRequestMagic = 0x504F5254;  // "PORT"

// Samples in a row before a release counts. Pressing needs no debouncing at
// second-long thresholds, but one bounce mid-hold would fire the short
// action.
constexpr int kReleaseSamples = 3;

// Time for the serial output to leave the buffer before the chip restarts.
constexpr uint32_t kFlushDelayMs = 200;

// Long enough for the red confirmation of ShowFactoryReset() to be seen.
constexpr uint32_t kFactoryResetConfirmMs = 1000;

// What a release right now would trigger.
ButtonHint HintFor(uint32_t held_ms) {
  if (held_ms >= kButtonFactoryResetHoldMs) {
    return ButtonHint::kFactoryReset;
  }
  if (held_ms >= kButtonPortalHoldMs) {
    return ButtonHint::kPortal;
  }
  return ButtonHint::kNone;
}

void HandleRelease(uint32_t held_ms) {
  if (held_ms >= kButtonFactoryResetHoldMs) {
    Serial.println("Button: factory reset, restarting");
    SettingsStore::Instance().Clear();
    WiFi.disconnect(/*wifioff=*/true, /*eraseap=*/true);
    ShowFactoryReset();
    delay(kFactoryResetConfirmMs);
    ESP.restart();
  } else if (held_ms >= kButtonPortalHoldMs) {
    Serial.println("Button: opening setup portal, restarting");
    portal_request = kPortalRequestMagic;
    delay(kFlushDelayMs);
    ESP.restart();
  }
}

void ButtonTask(void* /*unused*/) {
  pinMode(kButtonPin, INPUT_PULLUP);
  uint32_t pressed_since = 0;
  bool was_pressed = false;
  int released_samples = 0;
  for (;;) {
    const bool pressed = digitalRead(kButtonPin) == LOW;
    const uint32_t now = millis();
    if (pressed && !was_pressed) {
      pressed_since = now;
    }
    if (pressed) {
      released_samples = 0;
      SetButtonHint(HintFor(now - pressed_since));
    } else if (was_pressed && ++released_samples >= kReleaseSamples) {
      was_pressed = false;
      released_samples = 0;
      SetButtonHint(ButtonHint::kNone);
      HandleRelease(now - pressed_since);
    }
    was_pressed = was_pressed || pressed;
    vTaskDelay(pdMS_TO_TICKS(kButtonTickMs));
  }
}

}  // namespace

void StartButtonTask() {
  // 4 KB because SettingsStore::Clear() writes to NVS.
  xTaskCreatePinnedToCore(ButtonTask, "button", 4096, nullptr, 1, nullptr, 1);
}

bool PortalRequested() {
  const bool requested = portal_request == kPortalRequestMagic;
  portal_request = 0;
  return requested;
}

}  // namespace roomsense
