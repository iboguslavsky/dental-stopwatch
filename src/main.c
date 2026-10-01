#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_random.h"

#include "config.h"
#include "display_gc9a01.h"
#include "graphics.h"
#include "celebrations.h"
#include "button.h"
#include "wifi_ntp.h"
#include "ha_client.h"

static const char *TAG = "APP_MAIN";

void app_main(void)
{
    ESP_LOGI(TAG, "=== ESP32-C3 Dental Stopwatch & Clock Starting ===");

    // 1. Initialize Display & allocate DMA Framebuffer memory
    ESP_ERROR_CHECK(display_gc9a01_init());

    // 2. Initialize Graphics Engine & Ubuntu Fonts
    graphics_init();

    // 3. Initialize BOOT Button (GPIO 9)
    ESP_ERROR_CHECK(button_init());

    // 4. Initialize WiFi and NTP background service
    wifi_ntp_init();

    // 5. Initialize Home Assistant REST client for outside temperature
    ha_client_init();

    // 6. Initialize UI State
    ui_state_t state;
    memset(&state, 0, sizeof(state));
    state.mode = UI_MODE_CLOCK;
    state.timer_total_ms = TIMER_TOTAL_SECONDS * 1000;
    state.celebration_index = esp_random() % CELEBRATION_COUNT;

    int64_t timer_start_us = 0;
    int64_t done_start_us  = 0;
    uint32_t frame_count   = 0;

    ESP_LOGI(TAG, "Entering main UI loop. Press BOOT button (GPIO 9) to start 2-min timer.");

    while (1) {
        int64_t loop_start_us = esp_timer_get_time();

        // -------------------------------------------------------------
        // Check Hardware BOOT Button
        // -------------------------------------------------------------
        button_event_t btn = button_poll();
        if (btn == BTN_SHORT_PRESS) {
            if (state.mode == UI_MODE_CLOCK) {
                ESP_LOGI(TAG, "Starting 2-min dental timer!");
                state.mode = UI_MODE_TIMER;
                state.timer_elapsed_ms = 0;
                timer_start_us = esp_timer_get_time();
            } else if (state.mode == UI_MODE_TIMER) {
                ESP_LOGI(TAG, "Resetting 2-min timer to beginning!");
                state.timer_elapsed_ms = 0;
                timer_start_us = esp_timer_get_time();
            } else if (state.mode == UI_MODE_TIMER_DONE) {
                // Short-press cycles through celebration animations for playful testing
                state.celebration_index = (state.celebration_index + 1) % CELEBRATION_COUNT;
                done_start_us = esp_timer_get_time();
                ESP_LOGI(TAG, "Cycling to celebration animation #%d", state.celebration_index);
            }
        } else if (btn == BTN_LONG_PRESS) {
            if (state.mode == UI_MODE_CLOCK) {
                // Handy shortcut: Long-press in clock mode jumps directly to celebration for testing!
                ESP_LOGI(TAG, "Test shortcut: Long-press in clock mode triggered celebration test!");
                state.mode = UI_MODE_TIMER_DONE;
                done_start_us = esp_timer_get_time();
            } else if (state.mode == UI_MODE_TIMER || state.mode == UI_MODE_TIMER_DONE) {
                ESP_LOGI(TAG, "Timer / celebration cancelled by long-press. Returning to clock.");
                state.mode = UI_MODE_CLOCK;
            }
        }

        // -------------------------------------------------------------
        // Update UI State Machine
        // -------------------------------------------------------------
        get_current_local_time(&state.current_time);
        state.wifi_connected = wifi_is_connected();
        state.ntp_synced     = ntp_is_synced();
        state.anim_frame     = frame_count++;
        state.has_outdoor_temp = ha_client_get_outdoor_temp(&state.outdoor_temp_c);

        if (state.mode == UI_MODE_TIMER) {
            int64_t now_us = esp_timer_get_time();
            uint32_t elapsed_ms = (uint32_t)((now_us - timer_start_us) / 1000);

            if (elapsed_ms >= state.timer_total_ms) {
                // Select a random celebration animation from the 10 options
                state.celebration_index = esp_random() % CELEBRATION_COUNT;
                ESP_LOGI(TAG, "2-min brushing timer completed! Showing celebration #%d.", state.celebration_index);
                state.mode = UI_MODE_TIMER_DONE;
                state.timer_elapsed_ms = state.timer_total_ms;
                done_start_us = now_us;
            } else {
                state.timer_elapsed_ms = elapsed_ms;
            }
        } else if (state.mode == UI_MODE_TIMER_DONE) {
            int64_t now_us = esp_timer_get_time();
            uint32_t done_elapsed_ms = (uint32_t)((now_us - done_start_us) / 1000);
            if (done_elapsed_ms >= 10000) { // 10-second celebration animation
                ESP_LOGI(TAG, "Done celebration finished. Returning to clock mode.");
                state.mode = UI_MODE_CLOCK;
            }
        }

        // -------------------------------------------------------------
        // Render Frame directly into RAM Framebuffer and Stream via DMA
        // -------------------------------------------------------------
        uint16_t *fb = display_get_framebuffer();
        graphics_render_frame(fb, &state);
        display_flush();

        // -------------------------------------------------------------
        // Frame pacing
        // Target: ~30 FPS in timer and celebration modes (33ms), ~20 FPS in clock mode (50ms)
        // -------------------------------------------------------------
        int target_frame_ms = (state.mode == UI_MODE_CLOCK) ? 50 : 33;
        int64_t render_dur_ms = (esp_timer_get_time() - loop_start_us) / 1000;
        int remaining_delay_ms = target_frame_ms - (int)render_dur_ms;
        if (remaining_delay_ms > 0) {
            vTaskDelay(pdMS_TO_TICKS(remaining_delay_ms));
        } else {
            vTaskDelay(pdMS_TO_TICKS(1)); // Yield to prevent task starvation
        }
    }
}
