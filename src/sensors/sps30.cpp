#include "sensors/sps30.h"

#include <cmath>
#include <cstdint>

#include <sps30.h>

#include "core/readings.h"

namespace roomsense {
namespace {

// The sensor delivers a value every second. Going silent for much longer means
// it stopped measuring, e.g. after a power glitch. Generous enough to cover
// the 10 s automatic fan cleaning.
constexpr int kMaxPollsWithoutData = 15;

}  // namespace

const char* Sps30::Name() const { return "SPS30"; }

bool Sps30::DoInit() {
  if (!bus_initialized_) {
    sensirion_i2c_init();
    bus_initialized_ = true;
  }
  if (sps30_probe() != 0) {
    return false;
  }
  if (sps30_set_fan_auto_cleaning_interval_days(4) != 0) {
    return false;
  }
  // Starting is only accepted in idle mode. Stop first so this also works when
  // the sensor is still measuring, e.g. after a reset of the ESP32 alone.
  // Stopping while idle fails and is fine.
  sps30_stop_measurement();
  if (sps30_start_measurement() != 0) {
    return false;
  }
  polls_without_data_ = 0;
  return true;
}

bool Sps30::DoRead() {
  uint16_t data_ready = 0;
  if (sps30_read_data_ready(&data_ready) != 0) {
    return false;
  }
  if (data_ready == 0) {
    return ++polls_without_data_ < kMaxPollsWithoutData;
  }
  polls_without_data_ = 0;

  sps30_measurement measurement = {};
  if (sps30_read_measurement(&measurement) != 0) {
    return false;
  }
  mc_1p0_ = measurement.mc_1p0;
  mc_2p5_ = measurement.mc_2p5;
  mc_4p0_ = measurement.mc_4p0;
  mc_10p0_ = measurement.mc_10p0;
  return true;
}

void Sps30::Apply(Readings& readings) const {
  readings.pm_1 = mc_1p0_;
  readings.pm_2_5 = mc_2p5_;
  readings.pm_4 = mc_4p0_;
  readings.pm_10 = mc_10p0_;
  readings.sps_ok = IsOk();
}

void Sps30::Invalidate() {
  mc_1p0_ = NAN;
  mc_2p5_ = NAN;
  mc_4p0_ = NAN;
  mc_10p0_ = NAN;
}

}  // namespace roomsense
