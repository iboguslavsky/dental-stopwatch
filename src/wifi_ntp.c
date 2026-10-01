#include "wifi_ntp.h"
#include "config.h"
#include <string.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_sntp.h"

static const char *TAG = "WIFI_NTP";

static bool s_wifi_connected = false;
static bool s_ntp_synced = false;

static void time_sync_notification_cb(struct timeval *tv)
{
    s_ntp_synced = true;
    ESP_LOGI(TAG, "NTP Time successfully synchronized!");
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "WiFi station started, connecting to %s...", WIFI_SSID);
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        s_wifi_connected = false;
        ESP_LOGW(TAG, "WiFi disconnected, retrying in 2 seconds...");
        vTaskDelay(pdMS_TO_TICKS(2000));
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "WiFi connected! IP: " IPSTR, IP2STR(&event->ip_info.ip));
        s_wifi_connected = true;

        // Initialize SNTP if not already running
        if (!esp_sntp_enabled()) {
            ESP_LOGI(TAG, "Starting SNTP service...");
            esp_sntp_setoperatingmode(ESP_SNTP_OPMODE_POLL);
            esp_sntp_setservername(0, NTP_SERVER1);
            esp_sntp_setservername(1, NTP_SERVER2);
            esp_sntp_set_time_sync_notification_cb(time_sync_notification_cb);
            esp_sntp_init();
        }
    }
}

esp_err_t wifi_ntp_init(void)
{
    // Configure POSIX Timezone
    setenv("TZ", DEFAULT_TIMEZONE, 1);
    tzset();

    // 1. Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 2. Initialize TCP/IP and event loop
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    // 3. Configure WiFi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    wifi_config_t wifi_config = {0};
    strncpy((char *)wifi_config.sta.ssid, WIFI_SSID, sizeof(wifi_config.sta.ssid));
    strncpy((char *)wifi_config.sta.password, WIFI_PASSWORD, sizeof(wifi_config.sta.password));
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi & SNTP subsystem initialized");
    return ESP_OK;
}

bool wifi_is_connected(void)
{
    return s_wifi_connected;
}

bool ntp_is_synced(void)
{
    if (s_ntp_synced) return true;
    if (esp_sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED) {
        s_ntp_synced = true;
        return true;
    }
    return false;
}

void get_current_local_time(struct tm *timeinfo)
{
    time_t now;
    time(&now);
    localtime_r(&now, timeinfo);
}
