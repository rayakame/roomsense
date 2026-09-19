#pragma once

#include <cstdint>

#include <Arduino.h>

namespace roomsense {

// True once `now` has reached `deadline`, both millis() timestamps. Survives
// the wrap while the two are less than 24.8 days apart. Prefer Deadline.
inline bool Reached(uint32_t now, uint32_t deadline) {
  return static_cast<int32_t>(now - deadline) >= 0;
}

// A deadline that stays in the past once it passes. A plain timestamp does
// not: millis() wraps after 49.7 days, so anything older than 24.8 days looks
// like the future again and whoever waits for it stalls. Elapsed() disarms on
// the way through, which keeps the gap short. Poll it at least daily.
class Deadline {
 public:
  // Unarmed means elapsed.
  void ArmIn(uint32_t delay_ms) {
    at_ = millis() + delay_ms;
    armed_ = true;
  }

  // Let the next Elapsed() through.
  void Expire() { armed_ = false; }

  bool Elapsed() {
    if (armed_ && Reached(millis(), at_)) {
      armed_ = false;
    }
    return !armed_;
  }

 private:
  uint32_t at_ = 0;
  bool armed_ = false;
};

}  // namespace roomsense
