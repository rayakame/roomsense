#pragma once

#include <cstdint>

#include "core/readings.h"

namespace roomsense {

class Sensor {
 public:
  Sensor() = default;
  Sensor(const Sensor&) = delete;
  Sensor& operator=(const Sensor&) = delete;
  virtual ~Sensor() = default;

  // One cycle: initializes the sensor or takes a measurement, depending on
  // its state. Never blocks except during an initialization attempt.
  void Poll();

  bool IsOk() const { return state_ == State::kOk; }
  virtual const char* Name() const = 0;
  // Copies the last measurement into `readings`.
  virtual void Apply(Readings& readings) const = 0;

 protected:
  virtual bool DoInit() = 0;
  virtual bool DoRead() = 0;
  virtual void Invalidate() = 0;

 private:
  enum class State : std::uint8_t { kUninitialized, kOk, kFailed };

  void Fail();

  State state_ = State::kUninitialized;
  uint32_t next_init_attempt_ = 0;
};

}  // namespace roomsense
