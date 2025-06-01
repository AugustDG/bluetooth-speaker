#include "bt_audio.h"
#include "private/definitions.h"
#include "private/utils.h"
#include <esp_err.h>
#include <nvs.h>
#include <nvs_flash.h>

void bt_audio_config_init(bt_audio_config_t *config) {
  // Initialize default values
  config->name = "ESP32_BT_AUDIO";
  config->use_ssp = true;
  config->pin_len = 0;
  config->pin_type = ESP_BT_PIN_TYPE_VARIABLE;
}

esp_err_t bt_audio_driver_install(const bt_audio_config_t *config) {
  // initialize NVS (used for PHY calibration)
  esp_err_t res = nvs_flash_init();
  if (res == ESP_ERR_NVS_NO_FREE_PAGES ||
      res == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    // NVS partition was truncated and needs to be erased
    ESP_ERROR_CHECK(nvs_flash_erase());
    res = nvs_flash_init();
  }

  RETURN_ON_ERROR(res, BT_AUDIO_TAG, "Failed to initialize NVS");

  // release unused BLE resources
  res = esp_bt_controller_mem_release(ESP_BT_MODE_BLE);
  RETURN_ON_ERROR(res, BT_AUDIO_TAG, "Failed to release BLE memory resources");

  /* Controller */

  esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
  res = esp_bt_controller_init(&bt_cfg);
  RETURN_ON_ERROR(res, BT_AUDIO_TAG, "Failed to initialize BT controller");
  res = esp_bt_controller_mem_release(ESP_BT_MODE_BLE);
  RETURN_ON_ERROR(res, BT_AUDIO_TAG, "Failed to release BLE memory resources");

  /* Bluedroid */

  res = esp_bt_controller_init(&bt_cfg);
  RETURN_ON_ERROR(res, BT_AUDIO_TAG, "Failed to initialize BT controller");
  res = esp_bt_controller_enable(ESP_BT_MODE_BTDM);
  RETURN_ON_ERROR(res, BT_AUDIO_TAG, "Failed to enable BT controller");

  /* Bluedroid */

  res = esp_bluedroid_init();
  RETURN_ON_ERROR(res, BT_AUDIO_TAG, "Failed to initialize Bluedroid");
  res = esp_bluedroid_enable();
  RETURN_ON_ERROR(res, BT_AUDIO_TAG, "Failed to enable Bluedroid");

  /* Auth */

  if (config->use_ssp) {
    esp_bt_io_cap_t iocap = ESP_BT_IO_CAP_IO;
    res = esp_bt_gap_set_security_param(ESP_BT_SP_IOCAP_MODE, &iocap,
                                        sizeof(iocap));
    RETURN_ON_ERROR(res, BT_AUDIO_TAG, "Failed to setup security parameters");
  } else {
    esp_bt_pin_code_t pin_code;
    for (int i = 0; i < config->pin_len; i++) {
      pin_code[i] = config->pin_code[i];
    }

    res = esp_bt_gap_set_pin(config->pin_type, config->pin_len, pin_code);
    RETURN_ON_ERROR(res, BT_AUDIO_TAG, "Failed to set security pin");
  }

  /* GAP */

  if (!config->gap_callback) {
    return ESP_ERR_INVALID_ARG;
  }

  res = esp_bt_gap_register_callback(config->gap_callback);
  RETURN_ON_ERROR(res, BT_AUDIO_TAG, "Failed to register GAP callback");
  res = esp_bt_gap_set_device_name(config->name);
  RETURN_ON_ERROR(res, BT_AUDIO_TAG, "Failed to set device name");

  /* A2DP */

  if (!config->a2dp_callback || !config->a2dp_data_callback) {
    return ESP_ERR_INVALID_ARG;
  }

  res = esp_a2d_sink_init();
  RETURN_ON_ERROR(res, BT_AUDIO_TAG, "Failed to initialize A2DP sink");
  res = esp_a2d_register_callback(config->a2dp_callback);
  RETURN_ON_ERROR(res, BT_AUDIO_TAG, "Failed to register A2DP callback");
  res = esp_a2d_sink_register_data_callback(config->a2dp_data_callback);
  RETURN_ON_ERROR(res, BT_AUDIO_TAG, "Failed to register A2DP data callback");

  res =
      esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
  RETURN_ON_ERROR(res, BT_AUDIO_TAG, "Failed to set scan mode");

  return res;
}
