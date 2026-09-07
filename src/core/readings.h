#pragma once
#include <cmath>
#include <functional>
#include <mutex>


struct Readings {
    float temperature = NAN;
    float humidity = NAN;
    float lux = NAN;
    float pressure = NAN;
    float voc_index = NAN;
    float nox_index = NAN;
    float co2 = NAN;

    float pm_1 = NAN;
    float pm_2_5 = NAN;
    float pm_4 = NAN;
    float pm_10 = NAN;

    float noise_leq = NAN;
    float noise_max = NAN;

    bool sht_ok = false; // Humidity & Temp
    bool veml_ok = false; // Lux
    bool dps_ok = false; // Pressure
    bool sgp_ok = false; // VOx & NOx
    bool sps_ok = false; // PM
    bool scd_ok = false; // CO2
    bool mic_ok = false;

    uint32_t updated_at = 0;
};

class ReadingsStore {
public:
    void update(const std::function<void(Readings&)>& fn);
    Readings snapshot();
private:
    std::mutex mutex_;
    Readings data_;
};

extern ReadingsStore readings_store;
