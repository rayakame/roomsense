#include "sensors/sps30.h"

#include <cmath>
#include <cstdint>

#include <Arduino.h>
#include <sps30.h>

#include "core/readings.h"

namespace roomsense {

const char* Sps30::Name() const { return "SPS30"; }

bool Sps30::Init() {
  Serial.println("Initializing SPS30");

  sensirion_i2c_init();
  if (const int err = sps30_probe(); err != 0) {
    Serial.printf("SPS30 not found, error code: %d\n", err);
    Invalidate();
    return false;
  }
  if (const int err = sps30_set_fan_auto_cleaning_interval_days(4); err != 0) {
    Serial.printf("SPS30 error while configuring, error code: %d\n", err);
    Invalidate();
    return false;
  }
  if (const int err = sps30_start_measurement(); err != 0) {
    Serial.printf("SPS30 start failed, error code: %d\n", err);
    Invalidate();
    return false;
  }

  ok_ = true;
  return true;
}

bool Sps30::Read() {
  uint16_t pm_ready = 0;
  if (const int err = sps30_read_data_ready(&pm_ready); err != 0) {
    Invalidate();
    return false;
  }
  if (pm_ready == 0) {
    return true;
  }

  sps30_measurement measurement = {};
  if (const int err = sps30_read_measurement(&measurement); err != 0) {
    Invalidate();
    return false;
  }
  mc_1p0_ = measurement.mc_1p0;
  mc_2p5_ = measurement.mc_2p5;
  mc_4p0_ = measurement.mc_4p0;
  mc_10p0_ = measurement.mc_10p0;
  ok_ = true;
  return true;
}

void Sps30::Apply(Readings& readings) const {
  readings.pm_1 = mc_1p0_;
  readings.pm_2_5 = mc_2p5_;
  readings.pm_4 = mc_4p0_;
  readings.pm_10 = mc_10p0_;
  readings.sps_ok = ok_;
}

void Sps30::Invalidate() {
  mc_1p0_ = NAN;
  mc_2p5_ = NAN;
  mc_4p0_ = NAN;
  mc_10p0_ = NAN;
  ok_ = false;
}

}  // namespace roomsense
