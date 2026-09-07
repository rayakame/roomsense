#pragma once
#include "core/readings.h"

class Sensor {
public:
    virtual ~Sensor() = default;
    virtual const char* name() const = 0;
    virtual bool init() = 0;
    virtual bool read() = 0;
    virtual void apply(Readings& r) = 0;
};
