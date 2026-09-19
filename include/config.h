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
constexpr const char* kFirmwareVersion = "0.2.0";

// Network task timing.
constexpr uint32_t kNetTickMs = 100;
constexpr uint32_t kStatePublishIntervalMs = 10 * 1000;
// Identity, retained. Repeated so queries need no last_over_time().
constexpr uint32_t kInfoPublishIntervalMs = 10 * 60 * 1000;
// Runtime health. Heap and temperature move slowly; faster is just noise.
constexpr uint32_t kDiagPublishIntervalMs = 60 * 1000;
constexpr uint32_t kWifiRetryMs = 15 * 1000;
constexpr uint32_t kMqttRetryMs = 5 * 1000;

// The SGP41 learns its baseline over hours. Only usable after this much
// continuous operation, then written to NVS this often so a reflash keeps it.
constexpr uint32_t kVocBaselineMinRuntimeMs = 3 * 60 * 60 * 1000;
constexpr uint32_t kVocBaselineSaveIntervalMs = 60 * 60 * 1000;

// Setup button, wired between the pin and GND, internal pull-up enabled.
constexpr int kButtonPin = 10;
constexpr uint32_t kButtonTickMs = 20;
constexpr uint32_t kButtonPortalHoldMs = 2000;
constexpr uint32_t kButtonFactoryResetHoldMs = 5000;

}  // namespace roomsense
