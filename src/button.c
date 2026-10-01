#include "button.h"
#include "config.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"

static const char *TAG = "BUTTON";

#define DEBOUNCE_MIN_MS   25
#define LONG_PRESS_MS     800

static QueueHandle_t s_btn_queue = NULL;

static void button_task(void *pvParameters)
{
    int held_ms = 0;
    bool long_reported = false;

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10)); // 10ms sampling tick

        int level = gpio_get_level(PIN_BOOT_BTN);
        bool pressed = (level == 0); // Active LOW

        if (pressed) {
            held_ms += 10;
            if (held_ms >= LONG_PRESS_MS && !long_reported) {
                long_reported = true;
                button_event_t evt = BTN_LONG_PRESS;
                if (s_btn_queue) {
                    xQueueSend(s_btn_queue, &evt, 0);
                }
                ESP_LOGI(TAG, "BOOT Button Long Press detected (%d ms)", held_ms);
            }
        } else {
            // Button released
            if (held_ms >= DEBOUNCE_MIN_MS && !long_reported) {
                button_event_t evt = BTN_SHORT_PRESS;
                if (s_btn_queue) {
                    xQueueSend(s_btn_queue, &evt, 0);
                }
                ESP_LOGI(TAG, "BOOT Button Short Press detected (%d ms)", held_ms);
            }
            held_ms = 0;
            long_reported = false;
        }
    }
}

esp_err_t button_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << PIN_BOOT_BTN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    esp_err_t err = gpio_config(&io_conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure GPIO %d: %s", PIN_BOOT_BTN, esp_err_to_name(err));
        return err;
    }

    s_btn_queue = xQueueCreate(8, sizeof(button_event_t));
    if (!s_btn_queue) {
        ESP_LOGE(TAG, "Failed to create button queue");
        return ESP_ERR_NO_MEM;
    }

    BaseType_t ret = xTaskCreate(button_task, "btn_task", 2048, NULL, 5, NULL);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create button task");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "BOOT button initialized on GPIO %d (active LOW, FreeRTOS background 10ms debouncer)", PIN_BOOT_BTN);
    return ESP_OK;
}

button_event_t button_poll(void)
{
    button_event_t evt = BTN_NONE;
    if (s_btn_queue != NULL) {
        xQueueReceive(s_btn_queue, &evt, 0);
    }
    return evt;
}
