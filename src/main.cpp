#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SHT4x.h>
#include <Adafruit_VEML7700.h>
#include <Adafruit_DPS310.h>
#include <SensirionI2CSgp41.h>
#include <VOCGasIndexAlgorithm.h>
#include <NOxGasIndexAlgorithm.h>
#include <sps30.h>

Adafruit_SHT4x sht45;
Adafruit_VEML7700 veml;
Adafruit_DPS310 dps;
SensirionI2CSgp41 sgp41;
VOCGasIndexAlgorithm vocAlgo;
NOxGasIndexAlgorithm noxAlgo;

void setup() {
    Serial.begin(115200);
    Serial.setTxTimeoutMs(0);
    delay(2000);

    pinMode(I2C_POWER, OUTPUT);
    digitalWrite(I2C_POWER, HIGH);
    delay(100);

    Wire.begin();

    if (!sht45.begin(&Wire)) {
        Serial.println("SHT45 not found");
        while (true) delay(1000);
    }
    sht45.setPrecision(SHT4X_HIGH_PRECISION);
    sht45.setHeater(SHT4X_NO_HEATER);

    if (!veml.begin(&Wire)) {
        Serial.println("VEML7700 not found");
        while (true) delay(1000);
    }

    if (!dps.begin_I2C(DPS310_I2CADDR_DEFAULT, &Wire)) {
        Serial.println("DPS310 not found");
        while (true) delay(1000);
    }
    dps.configurePressure(DPS310_4HZ, DPS310_64SAMPLES);
    dps.configureTemperature(DPS310_4HZ, DPS310_64SAMPLES);

    sgp41.begin(Wire);
    uint16_t sgpSerial[3];
    if (sgp41.getSerialNumber(sgpSerial) != 0) {
        Serial.println("SGP41 not found");
        while (true) delay(1000);
    }

    // SPS30: probe, Auto-Cleaning alle 4 Tage, Messung starten
    sensirion_i2c_init();
    if (sps30_probe() != 0) {
        Serial.println("SPS30 not found");
        while (true) delay(1000);
    }
    sps30_set_fan_auto_cleaning_interval_days(4);
    if (sps30_start_measurement() < 0) {
        Serial.println("SPS30 start failed");
        while (true) delay(1000);
    }

    // SGP41 Conditioning: 10 s, nicht laenger!
    Serial.println("SGP41 conditioning...");
    uint16_t srawVoc = 0;
    for (int i = 0; i < 10; i++) {
        sgp41.executeConditioning(0x8000, 0x6666, srawVoc);
        delay(1000);
    }

    Serial.println("sensors ready");
}

void loop() {
    sensors_event_t hum, temp;
    sht45.getEvent(&hum, &temp);

    float lux = veml.readLux(VEML_LUX_AUTO);

    sensors_event_t dpsTemp, pressure;
    dps.getEvents(&dpsTemp, &pressure);

    uint16_t compRh = (uint16_t)(hum.relative_humidity * 65535.0f / 100.0f);
    uint16_t compT  = (uint16_t)((temp.temperature + 45.0f) * 65535.0f / 175.0f);
    uint16_t srawVoc = 0, srawNox = 0;
    int32_t vocIndex = 0, noxIndex = 0;
    if (sgp41.measureRawSignals(compRh, compT, srawVoc, srawNox) == 0) {
        vocIndex = vocAlgo.process(srawVoc);
        noxIndex = noxAlgo.process(srawNox);
    }

    struct sps30_measurement pm = {};
    uint16_t pmReady = 0;
    sps30_read_data_ready(&pmReady);
    if (pmReady) {
        sps30_read_measurement(&pm);
    }

    Serial.printf("T %.2f C  RH %.2f %%  Light %.1f lux  P %.2f hPa  VOC %ld  NOx %ld  "
                  "PM1 %.1f  PM2.5 %.1f  PM4 %.1f  PM10 %.1f ug/m3\n",
                  temp.temperature, hum.relative_humidity, lux, pressure.pressure,
                  vocIndex, noxIndex,
                  pm.mc_1p0, pm.mc_2p5, pm.mc_4p0, pm.mc_10p0);
    delay(1000);
}