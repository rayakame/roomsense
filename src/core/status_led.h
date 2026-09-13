#pragma once
#include <cstdint>

namespace roomsense {

enum class NetState : uint8_t { kDisconnected, kConnecting, kConnected };

// What the button will do if released now. Overrides everything else, so the
// user sees the effect of holding before committing to it.
enum class ButtonHint : uint8_t { kNone, kPortal, kFactoryReset };
void SetButtonHint(ButtonHint hint);

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
