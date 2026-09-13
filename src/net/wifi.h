#pragma once

namespace roomsense {

// Keeps the WiFi connection up. Call once per net tick, never blocks.
void WifiTick();
bool WifiIsConnected();
int WifiRssi();

}  // namespace roomsense