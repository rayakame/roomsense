#include "net/net_task.h"

#include <cstdint>

#include <Arduino.h>

#include "config.h"
#include "core/diagnostics.h"
#include "core/readings.h"
#include "core/time.h"
#include "net/mqtt.h"
#include "net/wifi.h"

namespace roomsense {
namespace {

void NetTask(void* /*unused*/) {
  Serial.println("Net: task started");
  RegisterTask(TaskId::kNet, xTaskGetCurrentTaskHandle());
  uint32_t next_state = 0;
  uint32_t next_info = 0;
  uint32_t next_diag = 0;
  bool was_connected = false;
  for (;;) {
    WifiTick();
    Mqtt::Instance().Tick();
    const uint32_t now = millis();

    const bool connected = Mqtt::Instance().IsConnected();
    if (connected && !was_connected) {
      // A fresh connection starts with identity and values; health follows
      // after the first full interval.
      next_info = now;
      next_state = now;
      next_diag = now + kDiagPublishIntervalMs;
    }
    was_connected = connected;

    if (connected) {
      if (Reached(now, next_info)) {
        Mqtt::Instance().PublishInfo();
        next_info = now + kInfoPublishIntervalMs;
      }
      if (Reached(now, next_state)) {
        Mqtt::Instance().PublishState(ReadingsStore::Instance().Snapshot());
        next_state = now + kStatePublishIntervalMs;
      }
      if (Reached(now, next_diag)) {
        Mqtt::Instance().PublishDiag();
        next_diag = now + kDiagPublishIntervalMs;
      }
    }
    vTaskDelay(pdMS_TO_TICKS(kNetTickMs));
  }
}

}  // namespace

void StartNetTask() { xTaskCreatePinnedToCore(NetTask, "net", 8192, nullptr, 1, nullptr, 1); }

}  // namespace roomsense
