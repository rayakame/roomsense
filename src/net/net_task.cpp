#include "net/net_task.h"

#include <cstdint>

#include <Arduino.h>

#include "config.h"
#include "core/diagnostics.h"
#include "core/factory_reset.h"
#include "core/readings.h"
#include "core/time.h"
#include "net/mqtt.h"
#include "net/wifi.h"

namespace roomsense {
namespace {

void NetTask(void* /*unused*/) {
  Serial.println("Net: task started");
  RegisterTask(TaskId::kNet, xTaskGetCurrentTaskHandle());
  Deadline next_state;
  Deadline next_info;
  Deadline next_diag;
  bool was_connected = false;
  for (;;) {
    // Before anything of ours is in flight.
    RunFactoryResetIfRequested();
    WifiTick();
    Mqtt::Instance().Tick();
    const bool connected = Mqtt::Instance().IsConnected();
    if (connected && !was_connected) {
      // A fresh connection starts with identity and values; health follows
      // after the first full interval.
      next_info.Expire();
      next_state.Expire();
      next_diag.ArmIn(kDiagPublishIntervalMs);
    }
    was_connected = connected;

    if (connected) {
      if (next_info.Elapsed()) {
        Mqtt::Instance().PublishInfo();
        next_info.ArmIn(kInfoPublishIntervalMs);
      }
      if (next_state.Elapsed()) {
        Mqtt::Instance().PublishState(ReadingsStore::Instance().Snapshot());
        next_state.ArmIn(kStatePublishIntervalMs);
      }
      if (next_diag.Elapsed()) {
        Mqtt::Instance().PublishDiag();
        next_diag.ArmIn(kDiagPublishIntervalMs);
      }
    }
    vTaskDelay(pdMS_TO_TICKS(kNetTickMs));
  }
}

}  // namespace

void StartNetTask() { xTaskCreatePinnedToCore(NetTask, "net", 8192, nullptr, 1, nullptr, 1); }

}  // namespace roomsense
