#include <Arduino.h>
#include <Wire.h>

#include "config.h"
#include "core/readings.h"
#include "sensors/registry.h"

namespace roomsense {

namespace {

void SensorTask(void* /*unused*/) {
  TickType_t last_wake = xTaskGetTickCount();
  for (;;) {
    for (Sensor* sensor : AllSensors()) {
      sensor->Read();
      ReadingsStore::Instance().Update([sensor](Readings& readings) { sensor->Apply(readings); });
    }
    ReadingsStore::Instance().Update([](Readings& readings) { readings.updated_at = millis(); });
    vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(kSensorIntervalMs));
  }
}

}  // namespace

}  // namespace roomsense

void setup() {
  Serial.begin(115200);
  Serial.setTxTimeoutMs(0);
  delay(2000);

  pinMode(I2C_POWER, OUTPUT);
  digitalWrite(I2C_POWER, HIGH);
  delay(100);

  Wire.begin();

  for (roomsense::Sensor* sensor : roomsense::AllSensors()) {
    if (!sensor->Init()) {
      Serial.printf("Sensor %s failed to initialize!\n", sensor->Name());
    }
  }

  xTaskCreatePinnedToCore(roomsense::SensorTask, "sensors", 8192, nullptr, 1, nullptr, 1);
}

void loop() {
  const roomsense::Readings readings = roomsense::ReadingsStore::Instance().Snapshot();
  Serial.printf(
      "T %.2f  RH %.2f  lux %.1f  P %.2f  VOC %.0f  NOx %.0f  PM2.5 %.1f  ok: sht=%d veml=%d "
      "dps=%d sgp=%d sps=%d\n",
      readings.temperature, readings.humidity, readings.lux, readings.pressure, readings.voc_index,
      readings.nox_index, readings.pm_2_5, static_cast<int>(readings.sht_ok),
      static_cast<int>(readings.veml_ok), static_cast<int>(readings.dps_ok),
      static_cast<int>(readings.sgp_ok), static_cast<int>(readings.sps_ok));
  delay(2000);
}
