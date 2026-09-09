#pragma once
#include <cstdint>

namespace roomsense {

enum class NetState : uint8_t { kDisconnected, kConnecting, kConnected };


void StartStatusLedTask();

// The config portal is open and waits for the user.
void SetSetupActive(bool active);
// WiFi/MQTT state. Disconnected for more than kNetErrorAfterMs shows as error.
void SetNetworkState(NetState state);
// At least one sensor does not answer.
void SetSensorFault(bool fault);
// Nothing works anymore, e.g. NVS is broken.
void SetFatalError(bool error);
// Confirms a factory reset with one second of solid red.
void ShowFactoryReset();

}  // namespace roomsense
