#pragma once

#include "esp_err.h"

esp_err_t sdcard_init(const char *mount_path);
esp_err_t sdcard_deinit(void);