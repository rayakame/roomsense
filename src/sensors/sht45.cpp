#include "sensors/sht45.h"

#include <cmath>

#include <Adafruit_SHT4x.h>
#include <Adafruit_Sensor.h>
#include <Arduino.h>
#include <Wire.h>

#include "core/readings.h"

namespace roomsense {

const char* Sht45::Name() const { return "SHT45"; }

bool Sht45::Init() {
  Serial.println("Initializing SHT45");
  if (!sht_.begin(&Wire)) {
    Serial.println("SHT45 not found");
    Invalidate();
    return false;
  }
  sht_.setPrecision(SHT4X_HIGH_PRECISION);
  sht_.setHeater(SHT4X_NO_HEATER);
  ok_ = true;
  return true;
}

bool Sht45::Read() {
  sensors_event_t humidity = {};
  sensors_event_t temperature = {};
  if (!sht_.getEvent(&humidity, &temperature)) {
    Invalidate();
    return false;
  }
  temperature_ = temperature.temperature;
  humidity_ = humidity.relative_humidity;
  ok_ = true;
  return true;
}

void Sht45::Apply(Readings& readings) const {
  readings.temperature = temperature_;
  readings.humidity = humidity_;
  readings.sht_ok = ok_;
}

void Sht45::Invalidate() {
  temperature_ = NAN;
  humidity_ = NAN;
  ok_ = false;
}

}  // namespace roomsense
