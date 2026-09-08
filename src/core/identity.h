#pragma once

#include <string>

namespace roomsense {

// Six lowercase hex characters from the last three bytes of the factory MAC
// address, e.g. "a1b2c3". Stable for the lifetime of the chip.
const std::string& DeviceId();

// "roomsense-" followed by DeviceId(), e.g. "roomsense-a1b2c3".
const std::string& Hostname();

}  // namespace roomsense
