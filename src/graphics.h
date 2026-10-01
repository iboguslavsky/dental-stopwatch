#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    UI_MODE_CLOCK = 0,
    UI_MODE_TIMER,
    UI_MODE_TIMER_DONE
} ui_mode_t;

typedef struct {
    ui_mode_t mode;
    
    // Clock info
    struct tm current_time;
    bool wifi_connected;
    bool ntp_synced;
    
    // Timer info
    uint32_t timer_elapsed_ms;
    uint32_t timer_total_ms;
    
    // Frame count
    uint32_t anim_frame;

    // Outdoor Weather from Home Assistant
    bool has_outdoor_temp;
    float outdoor_temp_c;

    // Random Celebration Animation (0 to 9)
    uint8_t celebration_index;
} ui_state_t;

#define SWAP_U16(x) ((uint16_t)((((x) & 0xFF) << 8) | (((x) >> 8) & 0xFF)))

// Undefine raw colors from config.h to provide byte-swapped values for GC9A01 SPI DMA
#undef COLOR_BLACK
#undef COLOR_WHITE
#undef COLOR_DARK_BG
#undef COLOR_RING
#undef COLOR_TEXT_DIM
#undef COLOR_AMBER
#undef COLOR_RED
#undef COLOR_GREEN

// Common UI Colors (swapped for GC9A01 SPI DMA byte order)
#define COLOR_BLACK       0x0000
#define COLOR_WHITE       0xFFFF
#define COLOR_DARK_BG     SWAP_U16(0x0842) // Deep slate #080810
#define COLOR_GOLD        SWAP_U16(0xFEA0) // Elegant gold
#define COLOR_RING        SWAP_U16(0x39E7) // Subtle steel ring
#define COLOR_TEXT_DIM    SWAP_U16(0x7BEF) // Dim grey
#define COLOR_AMBER       SWAP_U16(0xFDA0) // Amber warning
#define COLOR_CYAN        SWAP_U16(0x07FF) // Bright cyan
#define COLOR_RED         SWAP_U16(0xF800) // Vibrant Red
#define COLOR_ORANGE      SWAP_U16(0xFC00) // Orange
#define COLOR_YELLOW      0x00FE           // Sunny Gold/Yellow
#define COLOR_GREEN       0x0C27           // Mint Green #20E060
#define COLOR_SKY_BLUE    0x9F06           // Sky Blue #00D2FF
#define COLOR_PINK        0xF4F9           // Neon Pink #FF3CA0
#define COLOR_PURPLE      SWAP_U16(0x901F) // Purple
#define COLOR_DEEP_BLUE   SWAP_U16(0x0014) // Deep Blue

/**
 * @brief Initialize graphics engine (precomputed tables, etc.)
 */
void graphics_init(void);

/**
 * @brief Render complete 240x240 frame directly into the RAM framebuffer.
 * Uses UbuntuMono fonts from ~/esphome and precomputed fast trigonometry.
 * 
 * @param fb Pointer to 240x240 RGB565 framebuffer in RAM
 * @param state Current UI state
 */
void graphics_render_frame(uint16_t *fb, const ui_state_t *state);

// Shared primitives exported for celebrations and UI modules
void draw_line(uint16_t *fb, int x0, int y0, int x1, int y1, uint16_t color);
void draw_circle(uint16_t *fb, int xm, int ym, int r, uint16_t color);
void draw_filled_circle(uint16_t *fb, int xm, int ym, int r, uint16_t color);
void draw_sparkle_star(uint16_t *fb, int cx, int cy, int size, uint16_t color);
void draw_confetti(uint16_t *fb, uint32_t frame);
void draw_smiling_tooth(uint16_t *fb, int cx, int cy);

void draw_text_large(uint16_t *fb, int center_x, int y, const char *str, uint16_t fg_color, bool outline);
void draw_text_medium(uint16_t *fb, int center_x, int y, const char *str, uint16_t color, bool outline);
void draw_text_small(uint16_t *fb, int center_x, int y, const char *str, uint16_t color, bool outline);

#ifdef __cplusplus
}
#endif
