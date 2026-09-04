#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SHT4x.h>
#include <Adafruit_VEML7700.h>

Adafruit_SHT4x sht45;
Adafruit_VEML7700 veml;

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
    Serial.println("sensors ready");
}

void loop() {
    sensors_event_t hum, temp;
    sht45.getEvent(&hum, &temp);
    float lux = veml.readLux(VEML_LUX_AUTO);
    Serial.printf("T %.2f C  RH %.2f %%  Light %.1f lux\n",
                  temp.temperature, hum.relative_humidity, lux);
    delay(2000);
}