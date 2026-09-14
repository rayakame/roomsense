#include "core/diagnostics.h"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>

#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace roomsense {
namespace {

constexpr std::size_t kTaskCount = static_cast<std::size_t>(TaskId::kCount);

std::array<std::atomic<TaskHandle_t>, kTaskCount> task_handles;

std::atomic<uint32_t> wifi_reconnects = 0;
std::atomic<uint32_t> mqtt_reconnects = 0;
std::atomic<uint32_t> publish_failures = 0;
std::atomic<uint32_t> sensor_retries = 0;

std::size_t IndexOf(TaskId task) { return static_cast<std::size_t>(task); }

}  // namespace

void RegisterTask(TaskId task, TaskHandle_t handle) { task_handles.at(IndexOf(task)) = handle; }

uint32_t TaskStackFreeBytes(TaskId task) {
  TaskHandle_t handle = task_handles.at(IndexOf(task)).load();
  if (handle == nullptr) {
    return 0;
  }
  return uxTaskGetStackHighWaterMark(handle);
}

void CountWifiReconnect() { ++wifi_reconnects; }
void CountMqttReconnect() { ++mqtt_reconnects; }
void CountPublishFailure() { ++publish_failures; }
void CountSensorRetry() { ++sensor_retries; }

uint32_t WifiReconnects() { return wifi_reconnects; }
uint32_t MqttReconnects() { return mqtt_reconnects; }
uint32_t PublishFailures() { return publish_failures; }
uint32_t SensorRetries() { return sensor_retries; }

const char* ResetReasonName() {
  switch (esp_reset_reason()) {
    case ESP_RST_POWERON:
      return "power_on";
    case ESP_RST_EXT:
      return "external";
    case ESP_RST_SW:
      return "software";
    case ESP_RST_PANIC:
      return "panic";
    case ESP_RST_INT_WDT:
      return "interrupt_watchdog";
    case ESP_RST_TASK_WDT:
      return "task_watchdog";
    case ESP_RST_WDT:
      return "watchdog";
    case ESP_RST_DEEPSLEEP:
      return "deep_sleep";
    case ESP_RST_BROWNOUT:
      return "brownout";
    case ESP_RST_SDIO:
      return "sdio";
    case ESP_RST_USB:
      return "usb";
    case ESP_RST_JTAG:
      return "jtag";
    case ESP_RST_EFUSE:
      return "efuse";
    case ESP_RST_PWR_GLITCH:
      return "power_glitch";
    case ESP_RST_CPU_LOCKUP:
      return "cpu_lockup";
    case ESP_RST_UNKNOWN:
      break;
  }
  return "unknown";
}

}  // namespace roomsense
