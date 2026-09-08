#include "sensors/registry.h"

#include <array>

#include "sensors/dps310.h"
#include "sensors/sgp41.h"
#include "sensors/sht45.h"
#include "sensors/sps30.h"
#include "sensors/veml7700.h"

namespace roomsense {

std::span<Sensor* const> AllSensors() {
  static Sht45 sht45;
  static Veml7700 veml7700;
  static Dps310 dps310;
  static Sgp41 sgp41;
  static Sps30 sps30;
  static std::array<Sensor*, 5> sensors{&sht45, &veml7700, &dps310, &sgp41, &sps30};
  return sensors;
}

}  // namespace roomsense
