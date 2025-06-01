#pragma once

#include <esp_err.h>
#include <esp_log.h>

#define RETURN_ON_ERROR(expr, tag, msg)                                        \
  do {                                                                         \
    esp_err_t err = (expr);                                                    \
    if (err != ESP_OK) {                                                       \
      ESP_LOGE(tag, "%s: %s", msg, esp_err_to_name(err));                      \
      return err;                                                              \
    }                                                                          \
  } while (0)