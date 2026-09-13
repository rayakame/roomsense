#include "net/net_task.h"

#include <cstdint>

#include <Arduino.h>

#include "config.h"
#include "core/readings.h"
#include "core/time.h"
#include "net/mqtt.h"
#include "net/wifi.h"

namespace roomsense {
namespace {

void NetTask(void* /*unused*/) {
  Serial.println("Net: task started");
  uint32_t next_publish = 0;
  for (;;) {
    WifiTick();
    Mqtt::Instance().Tick();
    const uint32_t now = millis();
    if (Mqtt::Instance().IsConnected() && Reached(now, next_publish)) {
      Mqtt::Instance().PublishState(ReadingsStore::Instance().Snapshot());
      next_publish = now + kStatePublishIntervalMs;
    }
    vTaskDelay(pdMS_TO_TICKS(kNetTickMs));
  }
}

}  // namespace

void StartNetTask() { xTaskCreatePinnedToCore(NetTask, "net", 8192, nullptr, 1, nullptr, 1); }

}  // namespace roomsense
