#pragma once

#include <cstdint>

namespace roomsense {

// Interval between two sensor read cycles. The SGP41 gas index algorithms
// assume exactly one sample per second.
constexpr uint32_t kSensorIntervalMs = 1000;

// How long a sensor waits after a failed init or read before it tries to
// initialize again.
constexpr uint32_t kSensorInitRetryMs = 5000;

}  // namespace roomsense
