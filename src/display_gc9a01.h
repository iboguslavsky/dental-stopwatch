#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TFT_PIXELS  (TFT_WIDTH * TFT_HEIGHT)
#define TFT_FB_SIZE (TFT_PIXELS * sizeof(uint16_t)) // 115,200 bytes

/**
 * @brief Initialize SPI bus, GPIO pins, GC9A01 LCD controller, and allocate
 * DMA-capable framebuffer memory.
 */
esp_err_t display_gc9a01_init(void);

/**
 * @brief Check if true hardware double-buffering is active.
 */
bool display_is_double_buffered(void);

/**
 * @brief Get pointer to the active offscreen drawing framebuffer in RAM.
 */
uint16_t *display_get_framebuffer(void);

/**
 * @brief Clear the drawing framebuffer with a 16-bit color.
 */
void display_clear(uint16_t color);

/**
 * @brief Transmit the completed framebuffer to the GC9A01 over 80MHz SPI DMA.
 * In double-buffered mode, this immediately swaps back and front buffers so CPU
 * can start drawing the next frame without stalling.
 */
esp_err_t display_flush(void);

/**
 * @brief Wait for the current SPI DMA transfer to complete.
 */
void display_wait_flush(void);

#ifdef __cplusplus
}
#endif
