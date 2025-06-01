#pragma once

#include <esp_a2dp_api.h>
#include <esp_bt.h>
#include <esp_bt_device.h>
#include <esp_bt_main.h>
#include <esp_err.h>
#include <esp_gap_bt_api.h>
#include <stdbool.h>

typedef struct {
  const char *name;

  esp_bt_pin_type_t pin_type; // type of pin (fixed, variable, etc.)
  esp_bt_pin_code_t pin_code; // pin code for pairing
  uint8_t pin_len;            // length of the pin code
  bool use_ssp;               // use Secure Simple Pairing (SSP) or not

  esp_bt_gap_cb_t gap_callback;              // callback for GAP events
  esp_a2d_cb_t a2dp_callback;                // callback for A2DP events
  esp_a2d_sink_data_cb_t a2dp_data_callback; // callback for A2DP data
} bt_audio_config_t;

void bt_audio_config_init(bt_audio_config_t *config);
esp_err_t bt_audio_driver_install(const bt_audio_config_t *config);