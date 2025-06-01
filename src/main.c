#include <driver/i2s_std.h>
#include <esp_err.h>
#include <esp_log.h>

#include "bt_audio.h"
#include "i2s_audio.h"

#define BT_SPEAKER_TAG ("BT_SPEAKER")

static i2s_audio_t i2s_audio;

void bt_gap_callback(esp_bt_gap_cb_event_t event,
                     esp_bt_gap_cb_param_t *param) {
  static const char *tag = "BT_GAP";
  // discovery, mode & connection events
  switch (event) {
  case ESP_BT_GAP_DISC_RES_EVT:
    ESP_LOGI(tag, "Device discovered: %s", param->disc_res.bda);
    break;
  case ESP_BT_GAP_MODE_CHG_EVT:
    ESP_LOGI(tag, "Bluetooth mode changed");
    break;
  case ESP_BT_GAP_ACL_CONN_CMPL_STAT_EVT:
    if (param->acl_conn_cmpl_stat.stat == ESP_BT_STATUS_SUCCESS) {
      ESP_LOGI(tag, "ACL connection completed successfully");
    } else {
      ESP_LOGE(tag, "ACL connection failed with status: %d",
               param->acl_conn_cmpl_stat.stat);
    }
    break;

  // config events
  case ESP_BT_GAP_CONFIG_EIR_DATA_EVT:
    if (param->config_eir_data.stat == ESP_OK) {
      ESP_LOGI(tag, "EIR data configured successfully");
    } else {
      ESP_LOGE(tag, "Failed to configure EIR data");
    }
    break;

  // Security events
  case ESP_BT_GAP_CFM_REQ_EVT:
    ESP_LOGI(tag, "Simple Pairing User Confirmation request passkey: %06lu",
             param->cfm_req.num_val);
    esp_bt_gap_ssp_confirm_reply(param->cfm_req.bda,
                                 true); // we always say yes
    break;
  case ESP_BT_GAP_AUTH_CMPL_EVT:
    if (param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS) {
      ESP_LOGI(tag, "Authentication completed successfully for %s",
               param->auth_cmpl.device_name);
    } else {
      ESP_LOGE(tag, "Authentication failed with status: %d",
               param->auth_cmpl.stat);
    }
    break;
  case ESP_BT_GAP_ENC_CHG_EVT:
    char *str_enc[3] = {"OFF", "E0", "AES"};
    uint8_t *bda = (uint8_t *)param->enc_chg.bda;
    ESP_LOGI(tag,
             "Encryption mode to [%02x:%02x:%02x:%02x:%02x:%02x] changed to %s",
             bda[0], bda[1], bda[2], bda[3], bda[4], bda[5],
             str_enc[param->enc_chg.enc_mode]);
    break;

  // name and properties events
  case ESP_BT_GAP_GET_DEV_NAME_CMPL_EVT:
    if (param->get_dev_name_cmpl.status == ESP_BT_STATUS_SUCCESS) {
      ESP_LOGI(tag, "Device name: %s", param->get_dev_name_cmpl.name);
    } else {
      ESP_LOGE(tag, "Failed to get device name");
    }
    break;
  default:
    ESP_LOGI(tag, "Unhandled GAP event: %d", event);
    break;
  }
}

void a2dp_callback(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param) {
  static const char *tag = "BT_A2DP";
  // handle A2DP events here
  switch (event) {
  case ESP_A2D_CONNECTION_STATE_EVT:
    if (param->conn_stat.state == ESP_A2D_CONNECTION_STATE_CONNECTED) {
      ESP_LOGI(tag, "Device connected");
    } else if (param->conn_stat.state ==
               ESP_A2D_CONNECTION_STATE_DISCONNECTED) {
      ESP_LOGI(tag, "Device disconnected");
    }
    break;
  case ESP_A2D_AUDIO_STATE_EVT:
    // handle audio state changes
    break;
  case ESP_A2D_AUDIO_CFG_EVT:
    esp_a2d_mcc_t *mcc = &param->audio_cfg.mcc;

    uint32_t sample_rate = 16000; // default sample rate
    int channel_count = 2;        // default channel count

    if (mcc->type != ESP_A2D_MCT_SBC) {
      ESP_LOGE(tag, "Unsupported codec type: %d", mcc->type);
      return;
    }
    ESP_LOGI(tag, "Audio codec configured: %d", mcc->type);

    char sbc_freq_chan_info = mcc->cie.sbc[0];

    if ((sbc_freq_chan_info >> 6) & 0x01) {
      sample_rate = 32000; // 32kHz
    } else if ((sbc_freq_chan_info >> 5) & 0x01) {
      sample_rate = 44100; // 44.1kHz
    } else if ((sbc_freq_chan_info >> 4) & 0x01) {
      sample_rate = 48000; // 48kHz
    }

    if ((sbc_freq_chan_info >> 4) & 0x01) {
      channel_count = 1; // mono
    }

    ESP_LOGI(tag, "Sample rate: %lu Hz, Channel count: %d", sample_rate,
             channel_count);
    ESP_ERROR_CHECK(i2s_audio_driver_reconfig(
        &i2s_audio, sample_rate, 16)); // reconfigure I2S with new sample rate
    break;
  default:
    ESP_LOGI(tag, "Unhandled A2DP event: %d", event);
    break;
  }
}

void a2dp_data_callback(const uint8_t *buf, uint32_t len) {
  ESP_ERROR_CHECK_WITHOUT_ABORT(
      i2s_channel_write(i2s_audio.i2s_tx, buf, len, NULL, portMAX_DELAY));
}

void app_main() {
  ESP_LOGI(BT_SPEAKER_TAG, "Installing I2S audio driver...");

  i2s_audio_config_t i2s_config;
  i2s_audio_config_init(&i2s_config);
  i2s_config.i2s_num = I2S_NUM_0;
  i2s_config.bclk_pin = GPIO_NUM_27;
  i2s_config.ws_pin = GPIO_NUM_26;
  i2s_config.dout_pin = GPIO_NUM_25;
  i2s_config.bits_per_sample = 32;

  ESP_ERROR_CHECK(i2s_audio_driver_install(&i2s_audio, &i2s_config));

  ESP_LOGI(BT_SPEAKER_TAG, "Installing Bluetooth audio driver...");

  bt_audio_config_t bt_config;
  bt_audio_config_init(&bt_config);
  bt_config.name = "Augusto's Speaker";
  bt_config.use_ssp = true;
  bt_config.gap_callback = bt_gap_callback;
  bt_config.a2dp_callback = a2dp_callback;
  bt_config.a2dp_data_callback = a2dp_data_callback;

  ESP_ERROR_CHECK(bt_audio_driver_install(&bt_config));
}