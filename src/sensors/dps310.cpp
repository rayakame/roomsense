#include "sensors/dps310.h"

#include <cmath>

#include <Adafruit_DPS310.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

#include "core/readings.h"
#include "sensors/i2c.h"

namespace roomsense {

const char* Dps310::Name() const { return "DPS310"; }

bool Dps310::DoInit() {
  if (!I2cDevicePresent(Wire, DPS310_I2CADDR_DEFAULT)) {
    return false;
  }
  if (!dps_.begin_I2C(DPS310_I2CADDR_DEFAULT, &Wire)) {
    return false;
  }
  dps_.configurePressure(DPS310_4HZ, DPS310_64SAMPLES);
  dps_.configureTemperature(DPS310_4HZ, DPS310_64SAMPLES);
  return true;
}

bool Dps310::DoRead() {
  // getEvents() does not report I2C errors, so check the bus explicitly.
  if (!I2cDevicePresent(Wire, DPS310_I2CADDR_DEFAULT)) {
    return false;
  }
  sensors_event_t temperature = {};
  sensors_event_t pressure = {};
  dps_.getEvents(&temperature, &pressure);
  pressure_ = pressure.pressure;
  temperature_ = temperature.temperature;
  return true;
}

void Dps310::Apply(Readings& readings) const {
  readings.pressure = pressure_;
  readings.dps_temperature = temperature_;
  readings.dps_ok = IsOk();
}

void Dps310::Invalidate() {
  pressure_ = NAN;
  temperature_ = NAN;
}

}  // namespace roomsense
