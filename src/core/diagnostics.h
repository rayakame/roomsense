#pragma once

#include <cstdint>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace roomsense {

// Tasks whose remaining stack is reported in the diagnostics message.
enum class TaskId : uint8_t { kSensor, kMic, kNet, kCount };

// Each task registers itself once, with its own handle.
void RegisterTask(TaskId task, TaskHandle_t handle);
// Smallest amount of free stack the task ever had, in bytes. 0 while the task
// has not registered itself.
uint32_t TaskStackFreeBytes(TaskId task);

// Counters, incremented where the event happens.
void CountWifiReconnect();
void CountMqttReconnect();
void CountPublishFailure();
void CountSensorRetry();

uint32_t WifiReconnects();
uint32_t MqttReconnects();
uint32_t PublishFailures();
uint32_t SensorRetries();

// Why the chip started the last time, e.g. "power_on" or "panic".
const char* ResetReasonName();

}  // namespace roomsense
