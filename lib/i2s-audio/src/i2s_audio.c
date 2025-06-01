#include "i2s_audio.h"
#include "private/definitions.h"
#include "private/utils.h"
#include <driver/i2s_std.h>
#include <esp_err.h>

void i2s_audio_config_init(i2s_audio_config_t *config) {
  config->i2s_num = I2S_NUM_0;
  config->mclk_pin = I2S_GPIO_UNUSED;
  config->bclk_pin = I2S_GPIO_UNUSED;
  config->ws_pin = I2S_GPIO_UNUSED;
  config->dout_pin = I2S_GPIO_UNUSED;
  config->sample_rate = 44100;
  config->bits_per_sample = 16;
}

esp_err_t i2s_audio_driver_install(i2s_audio_t *i2s_audio,
                                   const i2s_audio_config_t *config) {
  i2s_chan_config_t chan_cfg =
      I2S_CHANNEL_DEFAULT_CONFIG(config->i2s_num, I2S_ROLE_MASTER);
  chan_cfg.auto_clear = true;

  i2s_std_config_t std_cfg = {
      .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(config->sample_rate),
      .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(config->bits_per_sample,
                                                  I2S_SLOT_MODE_STEREO),
      .gpio_cfg =
          {
              .mclk = config->mclk_pin,
              .bclk = config->bclk_pin,
              .ws = config->ws_pin,
              .dout = config->dout_pin,
              .din = I2S_GPIO_UNUSED,
              .invert_flags =
                  {
                      .mclk_inv = false,
                      .bclk_inv = false,
                      .ws_inv = false,
                  },

          },
  };

  esp_err_t res = i2s_new_channel(&chan_cfg, &i2s_audio->i2s_tx, NULL);
  RETURN_ON_ERROR(res, I2S_AUDIO_TAG, "Failed to create I2S channel");

  res = i2s_channel_init_std_mode(i2s_audio->i2s_tx, &std_cfg);
  RETURN_ON_ERROR(res, I2S_AUDIO_TAG,
                  "Failed to initialize I2S channel in "
                  "standard mode");
  res = i2s_channel_enable(i2s_audio->i2s_tx);
  RETURN_ON_ERROR(res, I2S_AUDIO_TAG, "Failed to enable I2S channel");

  return ESP_OK;
}

esp_err_t i2s_audio_driver_reconfig(i2s_audio_t *i2s_audio,
                                    uint32_t sample_rate,
                                    uint8_t bits_per_sample) {
  if (i2s_audio == NULL || i2s_audio->i2s_tx == NULL) {
    return ESP_ERR_INVALID_ARG;
  }

  esp_err_t res = i2s_channel_disable(i2s_audio->i2s_tx);
  RETURN_ON_ERROR(res, I2S_AUDIO_TAG, "Failed to disable I2S channel");

  i2s_std_clk_config_t clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(sample_rate);
  i2s_std_slot_config_t slot_cfg =
      I2S_STD_MSB_SLOT_DEFAULT_CONFIG(bits_per_sample, I2S_SLOT_MODE_STEREO);

  res = i2s_channel_reconfig_std_clock(i2s_audio->i2s_tx, &clk_cfg);
  RETURN_ON_ERROR(res, I2S_AUDIO_TAG, "Failed to reconfigure sample rate");
  res = i2s_channel_reconfig_std_slot(i2s_audio->i2s_tx, &slot_cfg);
  RETURN_ON_ERROR(res, I2S_AUDIO_TAG, "Failed to reconfigure bits per sample");

  res = i2s_channel_enable(i2s_audio->i2s_tx);
  RETURN_ON_ERROR(res, I2S_AUDIO_TAG, "Failed to enable I2S channel");

  return ESP_OK;
}
