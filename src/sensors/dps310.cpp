#include "sensors/dps310.h"

#include <cmath>

#include <Adafruit_DPS310.h>
#include <Adafruit_Sensor.h>
#include <Arduino.h>
#include <Wire.h>

#include "core/readings.h"

namespace roomsense {

const char* Dps310::Name() const { return "DPS310"; }

bool Dps310::Init() {
  Serial.println("Initializing DPS310");
  if (!dps_.begin_I2C(DPS310_I2CADDR_DEFAULT, &Wire)) {
    Serial.println("DPS310 not found");
    Invalidate();
    return false;
  }
  dps_.configurePressure(DPS310_4HZ, DPS310_64SAMPLES);
  dps_.configureTemperature(DPS310_4HZ, DPS310_64SAMPLES);
  ok_ = true;
  return true;
}

bool Dps310::Read() {
  sensors_event_t temperature = {};
  sensors_event_t pressure = {};
  if (!dps_.getEvents(&temperature, &pressure)) {
    Invalidate();
    return false;
  }
  pressure_ = pressure.pressure;
  temperature_ = temperature.temperature;
  ok_ = true;
  return true;
}

void Dps310::Apply(Readings& readings) const {
  readings.pressure = pressure_;
  readings.dps_temperature = temperature_;
  readings.dps_ok = ok_;
}

void Dps310::Invalidate() {
  pressure_ = NAN;
  temperature_ = NAN;
  ok_ = false;
}

}  // namespace roomsense
