#pragma once

#include <stdbool.h>
#include <time.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize WiFi station and SNTP time synchronization.
 * Runs non-blocking in background FreeRTOS tasks.
 */
esp_err_t wifi_ntp_init(void);

/**
 * @brief Check if WiFi is currently connected with valid IP address.
 */
bool wifi_is_connected(void);

/**
 * @brief Check if time has been successfully synchronized with NTP.
 */
bool ntp_is_synced(void);

/**
 * @brief Get current local broken-down time.
 */
void get_current_local_time(struct tm *timeinfo);

#ifdef __cplusplus
}
#endif
