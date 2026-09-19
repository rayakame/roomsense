#include "audio/mic.h"

#include <array>
#include <cstddef>
#include <cstdint>

#include <Arduino.h>
#include <driver/i2s_std.h>

#include "audio/level_meter.h"
#include "config.h"
#include "core/diagnostics.h"

namespace roomsense {
namespace {

i2s_chan_handle_t rx_handle = nullptr;

// Hands the channel back. Safe to call without one, and the disable is allowed
// to fail: a channel that never started is already in the right state.
void StopI2s() {
  if (rx_handle == nullptr) {
    return;
  }
  i2s_channel_disable(rx_handle);
  i2s_del_channel(rx_handle);
  rx_handle = nullptr;
}

bool StartI2s() {
  const i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
  if (i2s_new_channel(&chan_cfg, nullptr, &rx_handle) != ESP_OK) {
    rx_handle = nullptr;
    return false;
  }

  i2s_std_config_t std_cfg = {
      .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(kMicSampleRate),
      .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_MONO),
      .gpio_cfg =
          {
              .mclk = I2S_GPIO_UNUSED,
              .bclk = static_cast<gpio_num_t>(kMicBclkPin),
              .ws = static_cast<gpio_num_t>(kMicWsPin),
              .dout = I2S_GPIO_UNUSED,
              .din = static_cast<gpio_num_t>(kMicDinPin),
              .invert_flags = {.mclk_inv = 0, .bclk_inv = 0, .ws_inv = 0},
          },
  };
  std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_LEFT;  // SEL auf GND

  if (i2s_channel_init_std_mode(rx_handle, &std_cfg) != ESP_OK ||
      i2s_channel_enable(rx_handle) != ESP_OK) {
    // Give the channel back, or the next attempt has nothing left to allocate.
    StopI2s();
    return false;
  }
  return true;
}

void MicTask(void* /*unused*/) {
  RegisterTask(TaskId::kMic, xTaskGetCurrentTaskHandle());
  static std::array<int32_t, kMicBlockSamples> block;
  static LevelMeter level_meter;

  bool running = false;
  bool failure_reported = false;
  int read_errors = 0;

  for (;;) {
    // Down: keep trying, the same way a missing I2C sensor does.
    if (!running) {
      if (StartI2s()) {
        Serial.println("MIC: ready");
        running = true;
        failure_reported = false;
        read_errors = 0;
        continue;
      }
      level_meter.Invalidate();
      // Only the first failure is worth a line, the retries would repeat it.
      if (!failure_reported) {
        Serial.printf("MIC: I2S start failed, retrying every %u s\n", kMicRetryMs / 1000);
        failure_reported = true;
      }
      vTaskDelay(pdMS_TO_TICKS(kMicRetryMs));
      continue;
    }

    std::size_t bytes_read = 0;
    const esp_err_t err = i2s_channel_read(rx_handle, block.data(), sizeof(block), &bytes_read,
                                           pdMS_TO_TICKS(kMicReadTimeoutMs));
    if (err == ESP_OK && bytes_read == sizeof(block)) {
      read_errors = 0;
      level_meter.Process(block);
      continue;
    }

    level_meter.Invalidate();
    if (++read_errors >= kMicMaxReadErrors) {
      Serial.printf("MIC: %d failed reads, restarting the channel\n", read_errors);
      StopI2s();
      running = false;
      continue;
    }
    // A read that fails without waiting would otherwise spin the task.
    vTaskDelay(pdMS_TO_TICKS(kMicReadTimeoutMs));
  }
}

}  // namespace

void StartMicTask() { xTaskCreatePinnedToCore(MicTask, "mic", 8192, nullptr, 2, nullptr, 0); }

}  // namespace roomsense
