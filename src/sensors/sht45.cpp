#include "sht45.h"
#include <Wire.h>

const char *Sht45::name() const {
    return "sht45";
}

bool Sht45::init() {
    Serial.println("Initializing SHT45");
    if (!sht_.begin(&Wire)) {
        Serial.println("SHT45 not found");
        return false;
    }
    sht_.setPrecision(SHT4X_HIGH_PRECISION);
    sht_.setHeater(SHT4X_NO_HEATER);
    ok_ = true;
    return true;
}

bool Sht45::read() {
    sensors_event_t hum, temp;
    if (!sht_.getEvent(&hum, &temp)) {
        ok_ = false;
        temperature_ = NAN;
        humidity_ = NAN;
        return false;
    }
    temperature_ = temp.temperature;
    humidity_ = hum.relative_humidity;
    ok_ = true;
    return true;
}

void Sht45::apply(Readings &r) {
    r.temperature = temperature_;
    r.humidity = humidity_;
    r.sht_ok = ok_;
}
