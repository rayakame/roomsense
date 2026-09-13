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

// Firmware version, published with every state message.
constexpr const char* kFirmwareVersion = "0.1.0";

// Network task timing.
constexpr uint32_t kNetTickMs = 100;
constexpr uint32_t kStatePublishIntervalMs = 10 * 1000;
constexpr uint32_t kWifiRetryMs = 15 * 1000;
constexpr uint32_t kMqttRetryMs = 5 * 1000;

}  // namespace roomsense
