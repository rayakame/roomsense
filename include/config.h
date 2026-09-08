#pragma once

#include <cstdint>

namespace roomsense {

// Interval between two sensor read cycles. The SGP41 gas index algorithms
// assume exactly one sample per second.
constexpr uint32_t kSensorIntervalMs = 1000;

}  // namespace roomsense
