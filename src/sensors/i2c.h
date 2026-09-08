#pragma once

#include <cstdint>

#include <Wire.h>

namespace roomsense {

// True if a device acknowledges `address` on the bus. Used by drivers whose
// library does not report I2C errors itself.
inline bool I2cDevicePresent(TwoWire& wire, uint8_t address) {
  wire.beginTransmission(address);
  return wire.endTransmission() == 0;
}

}  // namespace roomsense
