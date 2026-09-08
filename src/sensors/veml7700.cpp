#include "sensors/veml7700.h"

#include <cmath>

#include <Adafruit_VEML7700.h>
#include <Wire.h>

#include "core/readings.h"
#include "sensors/i2c.h"

namespace roomsense {

const char* Veml7700::Name() const { return "VEML7700"; }

bool Veml7700::DoInit() {
  if (!I2cDevicePresent(Wire, VEML7700_I2CADDR_DEFAULT)) {
    return false;
  }
  if (!driver_started_) {
    driver_started_ = veml_.begin(&Wire);
    return driver_started_;
  }
  // The sensor lost its configuration while it was disconnected. Apply the
  // same settings begin() uses.
  veml_.enable(false);
  veml_.interruptEnable(false);
  veml_.setPersistence(VEML7700_PERS_1);
  veml_.setGain(VEML7700_GAIN_1_8);
  veml_.setIntegrationTime(VEML7700_IT_100MS);
  veml_.powerSaveEnable(false);
  veml_.enable(true);
  return true;
}

bool Veml7700::DoRead() {
  if (!I2cDevicePresent(Wire, VEML7700_I2CADDR_DEFAULT)) {
    return false;
  }
  lux_ = veml_.readLux(VEML_LUX_AUTO);
  return true;
}

void Veml7700::Apply(Readings& readings) const {
  readings.lux = lux_;
  readings.veml_ok = IsOk();
}

void Veml7700::Invalidate() { lux_ = NAN; }

}  // namespace roomsense
