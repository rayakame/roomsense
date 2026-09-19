#pragma once

namespace roomsense {

// Asks for a factory reset. Only records the wish, so any task may call it.
void RequestFactoryReset();

// Wipes the settings and the WiFi credentials, then restarts. Does nothing
// until a reset was asked for, and never returns once one was. Call it from
// the network task only, at the top of its loop: that task owns the settings
// and the WiFi while the device runs, so nothing is halfway through using
// them at that point.
void RunFactoryResetIfRequested();

}  // namespace roomsense
