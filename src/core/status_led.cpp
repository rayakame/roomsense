#include "core/status_led.h"

#include <atomic>
#include <cstdint>

#include <Adafruit_NeoPixel.h>
#include <Arduino.h>

#include "core/time.h"

namespace roomsense {
namespace {

constexpr uint8_t kBrightness = 20;
constexpr uint32_t kTickMs = 50;
constexpr uint32_t kBootFlashMs = 500;
constexpr uint32_t kFactoryResetMs = 1000;
constexpr uint32_t kNetErrorAfterMs = 10 * 60 * 1000;

struct Rgb {
  uint8_t red;
  uint8_t green;
  uint8_t blue;
  friend bool operator==(const Rgb&, const Rgb&) = default;
};

constexpr Rgb kOff = {.red = 0, .green = 0, .blue = 0};
constexpr Rgb kWhite = {.red = 255, .green = 255, .blue = 255};
constexpr Rgb kBlue = {.red = 0, .green = 0, .blue = 255};
constexpr Rgb kYellow = {.red = 255, .green = 200, .blue = 0};
constexpr Rgb kOrange = {.red = 255, .green = 80, .blue = 0};
constexpr Rgb kRed = {.red = 255, .green = 0, .blue = 0};

std::atomic<bool> setup_active = false;
std::atomic<NetState> net_state = NetState::kDisconnected;
std::atomic<uint32_t> disconnected_since = 0;
std::atomic<bool> sensor_fault = false;
std::atomic<bool> fatal_error = false;
std::atomic<uint32_t> factory_reset_until = 0;
std::atomic<uint32_t> boot_until = 0;

// `color` for `on_ms` out of every `period_ms`, otherwise off.
Rgb Blink(uint32_t now, Rgb color, uint32_t period_ms, uint32_t on_ms) {
  return now % period_ms < on_ms ? color : kOff;
}

Rgb ColorFor(uint32_t now) {
  if (!Reached(now, factory_reset_until.load())) {
    return kRed;
  }
  if (!Reached(now, boot_until.load())) {
    return kWhite;
  }
  if (setup_active.load()) {
    return Blink(now, kBlue, 2000, 1000);
  }
  const NetState net = net_state.load();
  const bool net_error =
      net == NetState::kDisconnected && Reached(now, disconnected_since.load() + kNetErrorAfterMs);
  if (fatal_error.load() || net_error) {
    return Blink(now, kRed, 200, 100);
  }
  if (net != NetState::kConnected) {
    return Blink(now, kYellow, 500, 250);
  }
  if (sensor_fault.load()) {
    return Blink(now, kOrange, 5000, 100);
  }
  return kOff;
}

void StatusLedTask(void* /*unused*/) {
  pinMode(NEOPIXEL_POWER, OUTPUT);
  digitalWrite(NEOPIXEL_POWER, NEOPIXEL_POWER_ON);
  Adafruit_NeoPixel pixel(1, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);
  pixel.begin();
  pixel.setBrightness(kBrightness);

  Rgb shown = kOff;
  pixel.show();
  for (;;) {
    const Rgb color = ColorFor(millis());
    // show() briefly blocks interrupts, so only call it on a change.
    if (color != shown) {
      pixel.setPixelColor(0, Adafruit_NeoPixel::Color(color.red, color.green, color.blue));
      pixel.show();
      shown = color;
    }
    vTaskDelay(pdMS_TO_TICKS(kTickMs));
  }
}

}  // namespace

void StartStatusLedTask() {
  const uint32_t now = millis();
  boot_until = now + kBootFlashMs;
  disconnected_since = now;
  xTaskCreatePinnedToCore(StatusLedTask, "status_led", 3072, nullptr, 1, nullptr, 1);
}

void SetSetupActive(bool active) { setup_active = active; }

void SetNetworkState(NetState state) {
  if (state == NetState::kDisconnected && net_state.load() != NetState::kDisconnected) {
    disconnected_since = millis();
  }
  net_state = state;
}

void SetSensorFault(bool fault) { sensor_fault = fault; }

void SetFatalError(bool error) { fatal_error = error; }

void ShowFactoryReset() { factory_reset_until = millis() + kFactoryResetMs; }

}  // namespace roomsense
