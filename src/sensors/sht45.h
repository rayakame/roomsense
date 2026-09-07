#pragma once
#include "Adafruit_SHT4x.h"
#include "sensor.h"

class Sht45 : public Sensor {
public:
    const char* name() const override;
    bool init() override;
    bool read() override;
    void apply(Readings& r) override;
private:
    Adafruit_SHT4x sht_;
    float temperature_ = NAN;
    float humidity_ = NAN;
    bool ok_ = false;
};
