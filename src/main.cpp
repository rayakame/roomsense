#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SHT4x.h>

Adafruit_SHT4x sht45;

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
    Serial.printf("SHT45 serial: %08lx\n", sht45.readSerial());
}

void loop() {
    sensors_event_t hum, temp;
    sht45.getEvent(&hum, &temp);
    Serial.printf("T %.2f C  RH %.2f %%\n", temp.temperature, hum.relative_humidity);
    delay(2000);
}