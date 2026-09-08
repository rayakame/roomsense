#pragma once

#include <span>

#include "sensors/sensor.h"

namespace roomsense {

std::span<Sensor* const> AllSensors();

}
