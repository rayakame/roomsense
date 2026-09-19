#include "sensors/sensor.h"

#include <cstdint>

#include <Arduino.h>

#include "config.h"
#include "core/diagnostics.h"
#include "core/time.h"

namespace roomsense {
namespace {

constexpr int kRetrySeconds = static_cast<int>(kSensorInitRetryMs / 1000);

}  // namespace

void Sensor::Poll() {
  if (state_ == State::kOk) {
    if (!DoRead()) {
      Serial.printf("%s: read failed, reinitializing in %d s\n", Name(), kRetrySeconds);
      Fail();
    }
    return;
  }

  if (!next_init_attempt_.Elapsed()) {
    return;
  }
  // Counted here, where a new init really starts. A sensor that was never
  // found is not retrying, it is simply not installed.
  if (has_been_ok_) {
    CountSensorRetry();
  }
  if (DoInit()) {
    Serial.printf("%s: ready\n", Name());
    state_ = State::kOk;
    has_been_ok_ = true;
    return;
  }
  // Only report the first failure, further attempts would just repeat it.
  if (state_ == State::kUninitialized) {
    Serial.printf("%s: not found, retrying every %d s\n", Name(), kRetrySeconds);
  }
  Fail();
}

void Sensor::Fail() {
  Invalidate();
  state_ = State::kFailed;
  next_init_attempt_.ArmIn(kSensorInitRetryMs);
}

}  // namespace roomsense
