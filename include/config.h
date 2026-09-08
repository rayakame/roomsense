#pragma once

#include <cstdint>

namespace roomsense {

// Interval between two sensor read cycles. The SGP41 gas index algorithms
// assume exactly one sample per second.
constexpr uint32_t kSensorIntervalMs = 1000;

// How long a sensor waits after a failed init or read before it tries to
// initialize again.
constexpr uint32_t kSensorInitRetryMs = 5000;

// Microphone (ICS-43434 via I2S).
constexpr int kMicBclkPin = 5;
constexpr int kMicWsPin = 6;
constexpr int kMicDinPin = 9;
constexpr uint32_t kMicSampleRate = 48000;
constexpr int kMicBlockSamples = kMicSampleRate / 8;  // 125 ms
constexpr int kMicBlocksPerSecond = 8;

}  // namespace roomsense
