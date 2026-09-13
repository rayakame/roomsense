#pragma once

namespace roomsense {

// Watches the button on kButtonPin in its own task. Holding it for
// kButtonPortalHoldMs requests the config portal, kButtonFactoryResetHoldMs
// wipes the settings and the WiFi credentials. Both act on release and restart
// the device. While the button is held the LED shows what a release would do.
void StartButtonTask();

// True if the last restart was requested by the button, and clears the
// request. The flag lives in RTC memory: it survives a software restart but
// not a power loss, so a power cut never drops the device into the portal.
bool PortalRequested();

}  // namespace roomsense
