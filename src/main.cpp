#include "audio/mic.h"
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
      sensor->Poll();
      ReadingsStore::Instance().Update([sensor](Readings& readings) { sensor->Apply(readings); });
    }
    ReadingsStore::Instance().Update([](Readings& readings) { readings.updated_at = millis(); });
    if (xTaskDelayUntil(&last_wake, pdMS_TO_TICKS(kSensorIntervalMs)) == pdFALSE) {
      // An init attempt took longer than one interval. Start a fresh interval
      // instead of running the missed cycles back to back.
      last_wake = xTaskGetTickCount();
    }
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

  xTaskCreatePinnedToCore(roomsense::SensorTask, "sensors", 8192, nullptr, 1, nullptr, 1);
  roomsense::StartMicTask();
}

void loop() {
  const roomsense::Readings readings = roomsense::ReadingsStore::Instance().Snapshot();
  Serial.printf(
      "T %.2f  RH %.2f  lux %.1f  P %.2f  VOC %.0f  NOx %.0f  PM2.5 %.1f  Leq %.1f  Lmax %.1f  "
      "ok: sht=%d veml=%d dps=%d sgp=%d sps=%d mic=%d\n",
      readings.temperature, readings.humidity, readings.lux, readings.pressure, readings.voc_index,
      readings.nox_index, readings.pm_2_5, readings.noise_leq, readings.noise_max,
      static_cast<int>(readings.sht_ok), static_cast<int>(readings.veml_ok),
      static_cast<int>(readings.dps_ok), static_cast<int>(readings.sgp_ok),
      static_cast<int>(readings.sps_ok), static_cast<int>(readings.mic_ok));
  delay(2000);
}
