#pragma once

#include <driver/i2s_types.h>
#include <esp_a2dp_api.h>
#include <esp_err.h>
#include <stdbool.h>

typedef struct {
  uint8_t i2s_num;  // I2S port number (0 or 1)
  uint8_t mclk_pin; // MCLK pin number (I2S_GPIO_UNUSED if unused)
  uint8_t bclk_pin; // BCLK pin number (I2S_GPIO_UNUSED if unused)
  uint8_t ws_pin;   // WS pin number (I2S_GPIO_UNUSED if unused)
  uint8_t dout_pin; // DOUT pin number (I2S_GPIO_UNUSED if unused)

  uint32_t sample_rate;    // Sample rate in Hz (e.g., 44100)
  uint8_t bits_per_sample; // Bits per sample (e.g., 16, 24)
} i2s_audio_config_t;

typedef struct {
  i2s_chan_handle_t i2s_tx; // I2S channel handle for transmitting audio
} i2s_audio_t;

void i2s_audio_config_init(i2s_audio_config_t *config);
esp_err_t i2s_audio_driver_install(i2s_audio_t *i2s_audio,
                                   const i2s_audio_config_t *config);
esp_err_t i2s_audio_driver_reconfig(i2s_audio_t *i2s_audio,
                                    uint32_t sample_rate,
                                    uint8_t bits_per_sample);