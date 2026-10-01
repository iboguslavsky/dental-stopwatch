#pragma once

#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BTN_NONE = 0,
    BTN_SHORT_PRESS,
    BTN_LONG_PRESS
} button_event_t;

/**
 * @brief Initialize BOOT button on GPIO 9 with internal pullup.
 */
esp_err_t button_init(void);

/**
 * @brief Poll button state and return any detected event.
 * Call this periodically (e.g. every 10-20ms).
 */
button_event_t button_poll(void);

#ifdef __cplusplus
}
#endif
