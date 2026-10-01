#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "cJSON.h"

#include "config.h"
#include "wifi_ntp.h"
#include "ha_client.h"

static const char *TAG = "HA_CLIENT";

static float s_outdoor_temp_c = 0.0f;
static bool s_has_outdoor_temp = false;
static SemaphoreHandle_t s_temp_mutex = NULL;

static void ha_poll_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Home Assistant temperature polling task started");

    while (1) {
        // 1. Wait until Wi-Fi connection is established
        if (!wifi_is_connected()) {
            vTaskDelay(pdMS_TO_TICKS(2000));
            continue;
        }

        // 2. Validate token
        if (strcmp(HA_API_TOKEN, "PASTE_YOUR_HA_LONG_LIVED_ACCESS_TOKEN_HERE") == 0 ||
            strlen(HA_API_TOKEN) < 10) {
            static bool warned_once = false;
            if (!warned_once) {
                ESP_LOGW(TAG, "=========================================================");
                ESP_LOGW(TAG, "HA_API_TOKEN is not configured in include/config.h.");
                ESP_LOGW(TAG, "Please paste your Home Assistant Long-Lived Access Token");
                ESP_LOGW(TAG, "to enable live outdoor temperature sync.");
                ESP_LOGW(TAG, "=========================================================");
                warned_once = true;
            }
            vTaskDelay(pdMS_TO_TICKS(15000));
            continue;
        }

        // 3. Construct URL and Authorization header
        char base_url[128];
        strncpy(base_url, HA_BASE_URL, sizeof(base_url) - 1);
        base_url[sizeof(base_url) - 1] = '\0';

        char url[256];
        snprintf(url, sizeof(url), "%s/api/states/%s", base_url, HA_SENSOR_ENTITY_ID);

        char auth_header[512];
        snprintf(auth_header, sizeof(auth_header), "Bearer %s", HA_API_TOKEN);

        esp_http_client_config_t config = {
            .url = url,
            .timeout_ms = 6000,
            .method = HTTP_METHOD_GET,
            .is_async = false,
        };

        esp_http_client_handle_t client = esp_http_client_init(&config);
        if (client == NULL) {
            ESP_LOGE(TAG, "Failed to initialize HTTP client");
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }

        esp_http_client_set_header(client, "Authorization", auth_header);
        esp_http_client_set_header(client, "Content-Type", "application/json");

        esp_err_t err = esp_http_client_open(client, 0);
        if (err == ESP_OK) {
            esp_http_client_fetch_headers(client);
            int status_code = esp_http_client_get_status_code(client);

            if (status_code == 200) {
                char *response_buf = malloc(2048);
                if (response_buf != NULL) {
                    int read_len = esp_http_client_read_response(client, response_buf, 2047);
                    if (read_len > 0) {
                        response_buf[read_len] = '\0';
                        cJSON *root = cJSON_Parse(response_buf);
                        if (root) {
                            cJSON *state_item = cJSON_GetObjectItem(root, "state");
                            if (cJSON_IsString(state_item) && state_item->valuestring != NULL) {
                                const char *val_str = state_item->valuestring;
                                if (strcmp(val_str, "unavailable") != 0 && strcmp(val_str, "unknown") != 0) {
                                    float val = (float)atof(val_str);

                                    // Check if source sensor reported in Fahrenheit or Celsius
                                    bool is_f = true;
                                    cJSON *attrs = cJSON_GetObjectItem(root, "attributes");
                                    if (attrs) {
                                        cJSON *unit_item = cJSON_GetObjectItem(attrs, "unit_of_measurement");
                                        if (cJSON_IsString(unit_item) && unit_item->valuestring != NULL) {
                                            if (strchr(unit_item->valuestring, 'C') != NULL ||
                                                strchr(unit_item->valuestring, 'c') != NULL) {
                                                is_f = false;
                                            }
                                        }
                                    }

                                    // Convert to Celsius if reported in Fahrenheit
                                    float temp_c = is_f ? ((val - 32.0f) * 5.0f / 9.0f) : val;

                                    if (xSemaphoreTake(s_temp_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                                        s_outdoor_temp_c = temp_c;
                                        s_has_outdoor_temp = true;
                                        xSemaphoreGive(s_temp_mutex);
                                    }

                                    ESP_LOGI(TAG, "HA Outdoor Temp: %.1f °C (raw sensor: %.1f %s)",
                                             temp_c, val, is_f ? "°F" : "°C");
                                } else {
                                    ESP_LOGW(TAG, "HA Sensor state is: %s", val_str);
                                }
                            }
                            cJSON_Delete(root);
                        }
                    }
                    free(response_buf);
                }
            } else if (status_code == 401) {
                ESP_LOGE(TAG, "HA Authentication failed (401 Unauthorized). Verify HA_API_TOKEN in include/config.h");
            } else if (status_code == 404) {
                ESP_LOGE(TAG, "HA Entity not found (404). Verify HA_SENSOR_ENTITY_ID in include/config.h: %s", HA_SENSOR_ENTITY_ID);
            } else {
                ESP_LOGW(TAG, "HA HTTP response status: %d", status_code);
            }
            esp_http_client_close(client);
        } else {
            ESP_LOGW(TAG, "Failed to connect to HA at %s: %s", base_url, esp_err_to_name(err));
        }

        esp_http_client_cleanup(client);

        // Sleep: 60s if we already have a valid reading, 3s if still waiting for initial sync
        uint32_t delay_ms = s_has_outdoor_temp ? (HA_POLL_INTERVAL_SEC * 1000) : 3000;
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
}

void ha_client_init(void)
{
#if HA_ENABLED
    if (s_temp_mutex == NULL) {
        s_temp_mutex = xSemaphoreCreateMutex();
    }
    xTaskCreate(ha_poll_task, "ha_poll_task", 8192, NULL, 3, NULL);
    ESP_LOGI(TAG, "Home Assistant client initialized for sensor: %s", HA_SENSOR_ENTITY_ID);
#else
    ESP_LOGI(TAG, "Home Assistant client disabled via config");
#endif
}

bool ha_client_get_outdoor_temp(float *out_temp_c)
{
    if (!out_temp_c || !s_has_outdoor_temp) return false;

    if (xSemaphoreTake(s_temp_mutex, pdMS_TO_TICKS(20)) == pdTRUE) {
        *out_temp_c = s_outdoor_temp_c;
        xSemaphoreGive(s_temp_mutex);
        return true;
    }
    return false;
}
