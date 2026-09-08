#include "mic.h"

#include <array>

#include "audio/level_meter.h"
#include <Arduino.h>
#include <driver/i2s_std.h>

#include "config.h"

namespace roomsense {
namespace {

i2s_chan_handle_t rx_handle = nullptr;

bool InitI2s() {
  const i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
  if (i2s_new_channel(&chan_cfg, nullptr, &rx_handle) != ESP_OK) {
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

  if (i2s_channel_init_std_mode(rx_handle, &std_cfg) != ESP_OK) {
    return false;
  }
  return i2s_channel_enable(rx_handle) == ESP_OK;
}

void MicTask(void* /*unused*/) {
  static std::array<int32_t, kMicBlockSamples> block;
  static LevelMeter level_meter;

  if (!InitI2s()) {
    Serial.println("MIC: I2S init failed");
    level_meter.Invalidate();
    vTaskDelete(nullptr);
  }
  Serial.println("MIC: ready");

  for (;;) {
    std::size_t bytes_read = 0;
    const esp_err_t err =
        i2s_channel_read(rx_handle, block.data(), sizeof(block), &bytes_read, pdMS_TO_TICKS(500));
    if (err != ESP_OK || bytes_read != sizeof(block)) {
      level_meter.Invalidate();
      continue;
    }
    level_meter.Process(block);
  }
}

}  // namespace

void StartMicTask() { xTaskCreatePinnedToCore(MicTask, "mic", 8192, nullptr, 2, nullptr, 0); }

}  // namespace roomsense
