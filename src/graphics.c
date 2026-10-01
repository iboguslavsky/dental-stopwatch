#include "graphics.h"
#include "celebrations.h"
#include "ubuntu_fonts.h"
#include "angle_table.h"
#include "display_gc9a01.h"
#include "tooth_img.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const uint16_t C_BLACK       = COLOR_BLACK;
static const uint16_t C_WHITE       = COLOR_WHITE;
static const uint16_t C_DARK_BG     = COLOR_DARK_BG;
static const uint16_t C_GOLD        = COLOR_GOLD;
static const uint16_t C_RING        = COLOR_RING;
static const uint16_t C_TEXT_DIM    = COLOR_TEXT_DIM;
static const uint16_t C_AMBER       = COLOR_AMBER;
static const uint16_t C_WEDGE_ACC   = COLOR_CYAN;

// ---------------------------------------------------------------------------
// 4 Dental Quadrant Colors (Vibrant & cheerful for kids)
// ---------------------------------------------------------------------------
// Q1: Top-Right (0s - 30s) -> Electric Sky Blue
static const uint16_t C_Q1          = 0x9F06; // #00D2FF
static const uint16_t C_Q1_DARK     = 0xC608; // Deep navy background

// Q2: Bottom-Right (30s - 60s) -> Bright Mint Green
static const uint16_t C_Q2          = 0x0C27; // #20E060
static const uint16_t C_Q2_DARK     = 0x4209; // Deep forest green

// Q3: Bottom-Left (60s - 90s) -> Sunny Golden Amber
static const uint16_t C_Q3          = 0x00FE; // #FFC300
static const uint16_t C_Q3_DARK     = 0xE028; // Deep amber background

// Q4: Top-Left (90s - 120s) -> Electric Neon Pink
static const uint16_t C_Q4          = 0xF4F9; // #FF3CA0
static const uint16_t C_Q4_DARK     = 0x4420; // Deep purple background

// Rainbow palette for celebration animation
static const uint16_t C_RAINBOW[] = {
    SWAP_U16(0xF800), // Red
    SWAP_U16(0xFC00), // Orange
    0x00FE,           // Yellow/Gold
    0x0C27,           // Mint Green
    0x9F06,           // Sky Blue
    0xF4F9            // Neon Pink
};

static const char *MONTH_NAMES[] = {
    "JAN", "FEB", "MAR", "APR", "MAY", "JUN",
    "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"
};

static const char *DAY_NAMES[] = {
    "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"
};

void graphics_init(void)
{
    celebrations_init();
}

// ---------------------------------------------------------------------------
// Primitive Drawing Functions (Circles, Lines, Sparkles)
// ---------------------------------------------------------------------------

void draw_line(uint16_t *fb, int x0, int y0, int x1, int y1, uint16_t color)
{
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;

    while (1) {
        if (x0 >= 0 && x0 < TFT_WIDTH && y0 >= 0 && y0 < TFT_HEIGHT) {
            fb[y0 * TFT_WIDTH + x0] = color;
        }
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void draw_circle(uint16_t *fb, int xm, int ym, int r, uint16_t color)
{
    int x = -r, y = 0, err = 2 - 2 * r;
    do {
        int px1 = xm - x, px2 = xm + x;
        int py1 = ym + y, py2 = ym - y;
        if (py1 >= 0 && py1 < TFT_HEIGHT) {
            if (px1 >= 0 && px1 < TFT_WIDTH) fb[py1 * TFT_WIDTH + px1] = color;
            if (px2 >= 0 && px2 < TFT_WIDTH) fb[py1 * TFT_WIDTH + px2] = color;
        }
        if (py2 >= 0 && py2 < TFT_HEIGHT) {
            if (px1 >= 0 && px1 < TFT_WIDTH) fb[py2 * TFT_WIDTH + px1] = color;
            if (px2 >= 0 && px2 < TFT_WIDTH) fb[py2 * TFT_WIDTH + px2] = color;
        }
        r = err;
        if (r <= y) err += ++y * 2 + 1;
        if (r > x || err > y) err += ++x * 2 + 1;
    } while (x < 0);
}

void draw_filled_circle(uint16_t *fb, int xm, int ym, int r, uint16_t color)
{
    int r_sq = r * r;
    for (int dy = -r; dy <= r; dy++) {
        int py = ym + dy;
        if (py < 0 || py >= TFT_HEIGHT) continue;
        int row = py * TFT_WIDTH;
        int dy2 = dy * dy;
        for (int dx = -r; dx <= r; dx++) {
            if (dx * dx + dy2 <= r_sq) {
                int px = xm + dx;
                if (px >= 0 && px < TFT_WIDTH) {
                    fb[row + px] = color;
                }
            }
        }
    }
}

void draw_sparkle_star(uint16_t *fb, int cx, int cy, int size, uint16_t color)
{
    if (size <= 0) return;
    draw_line(fb, cx - size, cy, cx + size, cy, color);
    draw_line(fb, cx, cy - size, cx, cy + size, color);
    if (size >= 3) {
        int diag = size / 2;
        draw_line(fb, cx - diag, cy - diag, cx + diag, cy + diag, color);
        draw_line(fb, cx - diag, cy + diag, cx + diag, cy - diag, color);
    }
    if (cx >= 0 && cx < TFT_WIDTH && cy >= 0 && cy < TFT_HEIGHT) {
        fb[cy * TFT_WIDTH + cx] = C_WHITE;
    }
}

void draw_confetti(uint16_t *fb, uint32_t frame)
{
    for (int i = 0; i < 32; i++) {
        uint32_t seed = (uint32_t)i * 137 + 19;
        float base_ang = (float)(i * 11.25f) * ((float)M_PI / 180.0f);
        float speed = 0.9f + (float)(seed % 5) * 0.3f;
        float r = fmodf((float)frame * speed * 2.2f + (float)(seed % 50), 92.0f) + 16.0f;
        
        float cur_ang = base_ang + ((float)frame * 0.025f) * ((i % 2 == 0) ? 1.0f : -1.0f);
        int px = 120 + (int)(r * cosf(cur_ang));
        int py = 120 + (int)(r * sinf(cur_ang)) + (int)(fmodf((float)frame * 1.8f + (float)(seed % 25), 18.0f) - 9.0f);

        if (px >= 2 && px < TFT_WIDTH - 2 && py >= 2 && py < TFT_HEIGHT - 2) {
            uint16_t col = C_RAINBOW[(i + (frame / 8)) % 6];
            fb[py * TFT_WIDTH + px] = col;
            fb[py * TFT_WIDTH + px + 1] = col;
            fb[(py + 1) * TFT_WIDTH + px] = col;
            fb[(py + 1) * TFT_WIDTH + px + 1] = col;
        }
    }
}

void draw_smiling_tooth(uint16_t *fb, int cx, int cy)
{
    int start_x = cx - TOOTH_IMG_W / 2;
    int start_y = cy - TOOTH_IMG_H / 2;
    for (int y = 0; y < TOOTH_IMG_H; y++) {
        int py = start_y + y;
        if (py < 0 || py >= TFT_HEIGHT) continue;
        int row = py * TFT_WIDTH;
        for (int x = 0; x < TOOTH_IMG_W; x++) {
            int px = start_x + x;
            if (px < 0 || px >= TFT_WIDTH) continue;
            uint16_t col = s_tooth_img[y * TOOTH_IMG_W + x];
            if (col != 0x0000) {
                fb[row + px] = col;
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Ubuntu Font Renderers
// ---------------------------------------------------------------------------

static void draw_glyph(uint16_t *fb, int x, int y, const glyph_info_t *g, uint16_t color)
{
    if (!g || !g->bitmap) return;
    for (int r = 0; r < g->height; r++) {
        int py = y + r;
        if (py < 0 || py >= TFT_HEIGHT) continue;
        int row = py * TFT_WIDTH;
        const uint8_t *row_bytes = &g->bitmap[r * g->stride];

        for (int c = 0; c < g->width; c++) {
            int px = x + c;
            if (px < 0 || px >= TFT_WIDTH) continue;
            if (row_bytes[c / 8] & (0x80 >> (c % 8))) {
                fb[row + px] = color;
            }
        }
    }
}

static void draw_glyph_outline_2px(uint16_t *fb, int x, int y, const glyph_info_t *g, uint16_t outline_color)
{
    if (!g || !g->bitmap) return;
    for (int r = 0; r < g->height; r++) {
        const uint8_t *row_bytes = &g->bitmap[r * g->stride];
        for (int c = 0; c < g->width; c++) {
            if (row_bytes[c / 8] & (0x80 >> (c % 8))) {
                for (int dy = -2; dy <= 2; dy++) {
                    int py = y + r + dy;
                    if (py < 0 || py >= TFT_HEIGHT) continue;
                    int row = py * TFT_WIDTH;
                    for (int dx = -2; dx <= 2; dx++) {
                        int px = x + c + dx;
                        if (px >= 0 && px < TFT_WIDTH) {
                            fb[row + px] = outline_color;
                        }
                    }
                }
            }
        }
    }
}

void draw_text_large(uint16_t *fb, int center_x, int y, const char *str, uint16_t fg_color, bool outline)
{
    int len = (int)strlen(str);
    int total_w = len * 36;
    int cur_x = center_x - total_w / 2;

    if (outline) {
        int ox = cur_x;
        for (int i = 0; i < len; i++) {
            const glyph_info_t *g = ubuntu_large_get_glyph(str[i]);
            if (g) draw_glyph_outline_2px(fb, ox, y, g, C_BLACK);
            ox += 36;
        }
    }

    for (int i = 0; i < len; i++) {
        const glyph_info_t *g = ubuntu_large_get_glyph(str[i]);
        if (g) draw_glyph(fb, cur_x, y, g, fg_color);
        cur_x += 36;
    }
}

void draw_text_medium(uint16_t *fb, int center_x, int y, const char *str, uint16_t color, bool outline)
{
    int len = (int)strlen(str);
    int total_w = len * 11;
    int cur_x = center_x - total_w / 2;

    if (outline) {
        int ox = cur_x;
        for (int i = 0; i < len; i++) {
            const glyph_info_t *g = ubuntu_medium_get_glyph(str[i]);
            if (g) draw_glyph_outline_2px(fb, ox, y, g, C_BLACK);
            ox += 11;
        }
    }

    for (int i = 0; i < len; i++) {
        const glyph_info_t *g = ubuntu_medium_get_glyph(str[i]);
        if (g) draw_glyph(fb, cur_x, y, g, color);
        cur_x += 11;
    }
}

void draw_text_small(uint16_t *fb, int center_x, int y, const char *str, uint16_t color, bool outline)
{
    int len = (int)strlen(str);
    int total_w = len * 8;
    int cur_x = center_x - total_w / 2;

    if (outline) {
        int ox = cur_x;
        for (int i = 0; i < len; i++) {
            const glyph_info_t *g = ubuntu_small_get_glyph(str[i]);
            if (g) draw_glyph_outline_2px(fb, ox, y, g, C_BLACK);
            ox += 8;
        }
    }

    for (int i = 0; i < len; i++) {
        const glyph_info_t *g = ubuntu_small_get_glyph(str[i]);
        if (g) draw_glyph(fb, cur_x, y, g, color);
        cur_x += 8;
    }
}

static void draw_text_temp(uint16_t *fb, int center_x, int y, const char *str, uint16_t color, bool outline)
{
    int len = (int)strlen(str);
    int total_w = len * 24;
    int cur_x = center_x - total_w / 2;

    if (outline) {
        int ox = cur_x;
        for (int i = 0; i < len; i++) {
            const glyph_info_t *g = ubuntu_temp_get_glyph(str[i]);
            if (g) draw_glyph_outline_2px(fb, ox, y, g, C_BLACK);
            ox += 24;
        }
    }

    for (int i = 0; i < len; i++) {
        const glyph_info_t *g = ubuntu_temp_get_glyph(str[i]);
        if (g) draw_glyph(fb, cur_x, y, g, color);
        cur_x += 24;
    }
}

static uint16_t get_temperature_color(float temp_c)
{
    if (temp_c < 0.0f) {
        return SWAP_U16(0x05BF); // Electric Ice Blue (< 0°C)
    } else if (temp_c < 20.0f) {
        return 0x0C27;           // Fresh Mint Green (0°C - 20°C, C_Q2)
    } else if (temp_c <= 30.0f) {
        return SWAP_U16(0xFFE0); // Bright Vivid Yellow (20°C - 30°C)
    } else {
        return SWAP_U16(0xF940); // Blazing Hot Red-Orange (> 30°C)
    }
}

// ---------------------------------------------------------------------------
// Full Framebuffer Rendering Engine
// ---------------------------------------------------------------------------

void graphics_render_frame(uint16_t *fb, const ui_state_t *state)
{
    if (!fb || !state) return;

    // 1. Instant clear (< 0.1ms with memset)
    display_clear(COLOR_BLACK);

    if (state->mode == UI_MODE_TIMER) {
        // ===================================================================
        // 2-MINUTE DENTAL BRUSHING STOPWATCH WITH 4 QUADRANT COLORS
        // ===================================================================
        float progress = (float)state->timer_elapsed_ms / (float)state->timer_total_ms;
        if (progress > 1.0f) progress = 1.0f;
        if (progress < 0.0f) progress = 0.0f;

        uint16_t wedge_deg10 = (uint16_t)roundf(progress * 3600.0f);

        // Fill background with distinct colors per quadrant
        for (int y = 0; y < TFT_HEIGHT; y++) {
            int dy = y - 120;
            int dy_sq = dy * dy;
            int ady = abs(dy);
            if (ady >= 120) continue;
            int row = y * TFT_WIDTH;

            for (int x = 0; x < TFT_WIDTH; x++) {
                int dx = x - 120;
                int r_sq = dx * dx + dy_sq;
                if (r_sq > 118 * 118) continue; // black outside circle

                int adx = abs(dx);
                if (adx >= 120) continue;

                uint16_t q = s_quadrant_angle_x10[ady][adx];
                uint16_t ang10;
                uint16_t q_active_col;
                uint16_t q_dark_col;

                if (dx >= 0 && dy <= 0) {
                    // Quadrant 1 (Top-Right: 0° - 90°) -> Sky Blue
                    ang10 = q;
                    q_active_col = C_Q1;
                    q_dark_col   = C_Q1_DARK;
                } else if (dx >= 0 && dy > 0) {
                    // Quadrant 2 (Bottom-Right: 90° - 180°) -> Mint Green
                    ang10 = 1800 - q;
                    q_active_col = C_Q2;
                    q_dark_col   = C_Q2_DARK;
                } else if (dx < 0 && dy > 0) {
                    // Quadrant 3 (Bottom-Left: 180° - 270°) -> Sunny Gold
                    ang10 = 1800 + q;
                    q_active_col = C_Q3;
                    q_dark_col   = C_Q3_DARK;
                } else {
                    // Quadrant 4 (Top-Left: 270° - 360°) -> Neon Pink
                    ang10 = 3600 - q;
                    q_active_col = C_Q4;
                    q_dark_col   = C_Q4_DARK;
                }

                if (r_sq >= 114 * 114) {
                    // Outer quadrant border ring
                    fb[row + x] = (ang10 <= wedge_deg10) ? q_active_col : C_RING;
                } else {
                    // Circular dial area: filled wedge or unelapsed quadrant background
                    if (ang10 <= wedge_deg10) {
                        fb[row + x] = q_active_col;
                    } else {
                        fb[row + x] = q_dark_col;
                    }
                }
            }
        }

        // Quadrant divider crosshairs (white line dividers between the 4 zones)
        for (int r = 70; r <= 113; r++) {
            // 12 o'clock (0°)
            fb[(120 - r) * TFT_WIDTH + 120] = C_WHITE;
            // 6 o'clock (180°)
            fb[(120 + r) * TFT_WIDTH + 120] = C_WHITE;
            // 3 o'clock (90°)
            fb[120 * TFT_WIDTH + (120 + r)] = C_WHITE;
            // 9 o'clock (270°)
            fb[120 * TFT_WIDTH + (120 - r)] = C_WHITE;
        }

        // 30-degree minor ticks
        for (int i = 0; i < 12; i++) {
            if (i % 3 == 0) continue; // Major quadrant ticks already drawn above
            float rad = (i * 30.0f - 90.0f) * ((float)M_PI / 180.0f);
            float ca = cosf(rad), sa = sinf(rad);
            draw_line(fb, 120 + (int)(107.0f * ca), 120 + (int)(107.0f * sa),
                          120 + (int)(113.0f * ca), 120 + (int)(113.0f * sa), C_RING);
        }

        // Remaining time calculation
        uint32_t rem_ms = (state->timer_elapsed_ms < state->timer_total_ms)
                          ? (state->timer_total_ms - state->timer_elapsed_ms) : 0;
        uint32_t rem_sec = (rem_ms + 999) / 1000;
        if (rem_sec > TIMER_TOTAL_SECONDS) rem_sec = TIMER_TOTAL_SECONDS;

        char time_buf[16];
        snprintf(time_buf, sizeof(time_buf), "%d:%02d",
                 (int)(rem_sec / 60), (int)(rem_sec % 60));

        // Active dental quadrant details
        int quadrant = (int)(state->timer_elapsed_ms / 30000) + 1;
        if (quadrant > 4) quadrant = 4;

        const char *quad_names[] = {
            "Q1: UPPER RIGHT",
            "Q2: LOWER RIGHT",
            "Q3: LOWER LEFT",
            "Q4: UPPER LEFT"
        };
        // Large countdown numbers turn flashing amber in the final 10 seconds
        uint16_t digit_col = (rem_sec <= 10 && (state->anim_frame / 4) % 2 == 0) ? C_AMBER : C_WHITE;

        // Header: "BRUSHING TIMER" with 2-pixel black outline
        draw_text_medium(fb, 120, 62, "BRUSHING TIMER", C_WHITE, true);

        // Huge bold countdown digits in UbuntuMono-B 72pt with 2-pixel black outline
        draw_text_large(fb, 120, 94, time_buf, digit_col, true);

        // Active tooth quadrant banner in White with 2-pixel black outline (never blends with segment colors)
        draw_text_medium(fb, 120, 156, quad_names[quadrant - 1], C_WHITE, true);

    } else if (state->mode == UI_MODE_TIMER_DONE) {
        // Render one of the 10 playful celebrations selected randomly
        render_celebration(fb, state->celebration_index, state->anim_frame);
    } else {
        // ===================================================================
        // DIGITAL CLOCK MODE
        // ===================================================================
        for (int y = 0; y < TFT_HEIGHT; y++) {
            int dy = y - 120;
            int dy_sq = dy * dy;
            int row = y * TFT_WIDTH;
            for (int x = 0; x < TFT_WIDTH; x++) {
                int dx = x - 120;
                int r_sq = dx * dx + dy_sq;
                if (r_sq <= 113 * 113) {
                    fb[row + x] = C_DARK_BG;
                }
            }
        }

        // Elegant gold/accent bezel rings (matching outside_temp_display reference)
        draw_circle(fb, 120, 120, 117, C_GOLD);
        draw_circle(fb, 120, 120, 116, C_WHITE);
        draw_circle(fb, 120, 120, 115, C_GOLD);
        draw_circle(fb, 120, 120, 114, C_RING);

        // Dial tick marks (12 major hour ticks, 60 minute ticks)
        for (int i = 0; i < 60; i++) {
            float rad = (i * 6.0f - 90.0f) * ((float)M_PI / 180.0f);
            float ca = cosf(rad), sa = sinf(rad);
            if (i % 5 == 0) {
                float r_in = 104.0f, r_out = 113.0f;
                draw_line(fb, 120 + (int)(r_in * ca), 120 + (int)(r_in * sa),
                              120 + (int)(r_out * ca), 120 + (int)(r_out * sa), C_GOLD);
            } else {
                float r_in = 109.0f, r_out = 113.0f;
                draw_line(fb, 120 + (int)(r_in * ca), 120 + (int)(r_in * sa),
                              120 + (int)(r_out * ca), 120 + (int)(r_out * sa), C_RING);
            }
        }

        // Date in UbuntuMono-B 22pt: "THU, OCT 01"
        int wday = state->current_time.tm_wday;
        int mon  = state->current_time.tm_mon;
        if (wday < 0 || wday > 6) wday = 0;
        if (mon < 0 || mon > 11) mon = 0;
        char date_str[32];
        snprintf(date_str, sizeof(date_str), "%s, %s %02d",
                 DAY_NAMES[wday], MONTH_NAMES[mon], state->current_time.tm_mday);
        draw_text_medium(fb, 120, 46, date_str, C_GOLD, false);

        // Bold Time in 12-hour AM/PM format (UbuntuMono-B 72pt + AM/PM indicator)
        int hour12 = state->current_time.tm_hour % 12;
        if (hour12 == 0) hour12 = 12;
        bool is_pm = (state->current_time.tm_hour >= 12);
        const char *ampm_str = is_pm ? "PM" : "AM";

        char time_str[16];
        snprintf(time_str, sizeof(time_str), "%d:%02d", hour12, state->current_time.tm_min);

        int time_len = (int)strlen(time_str);
        int time_w = time_len * 36;
        int ampm_w = 16; // 2 chars * 8px in ubuntu_small
        int gap = 6;
        int total_time_w = time_w + gap + ampm_w;
        int start_time_x = 120 - total_time_w / 2;
        int time_center_x = start_time_x + time_w / 2;
        int ampm_center_x = start_time_x + time_w + gap + ampm_w / 2;

        draw_text_large(fb, time_center_x, 76, time_str, C_WHITE, true);
        draw_text_small(fb, ampm_center_x, 80, ampm_str, C_WEDGE_ACC, true);

        // Dynamic orbiting second dot on dial perimeter (keeps seconds progression visible)
        float s_rad = (state->current_time.tm_sec * 6.0f - 90.0f) * ((float)M_PI / 180.0f);
        int sec_x = 120 + (int)(108.0f * cosf(s_rad));
        int sec_y = 120 + (int)(108.0f * sinf(s_rad));
        draw_filled_circle(fb, sec_x, sec_y, 3, C_WEDGE_ACC);

        // Horizontal divider line visually separating Date/Time from Temperature
        draw_line(fb, 45, 135, 195, 135, C_RING);
        draw_line(fb, 60, 136, 180, 136, C_RING);

        // Outdoor Temperature in Celsius in large 48pt UbuntuMono-B (color-coded by temperature)
        if (!state->wifi_connected) {
            draw_text_small(fb, 120, 154, "WIFI SEARCHING", C_AMBER, false);
        } else if (!state->ntp_synced) {
            draw_text_small(fb, 120, 154, "NTP SYNCING...", C_WEDGE_ACC, false);
        } else if (state->has_outdoor_temp) {
            int rounded_c = (int)roundf(state->outdoor_temp_c);
            char temp_buf[16];
            snprintf(temp_buf, sizeof(temp_buf), "%d*C", rounded_c);
            uint16_t temp_col = get_temperature_color(state->outdoor_temp_c);
            draw_text_temp(fb, 120, 148, temp_buf, temp_col, true);
        } else {
            draw_text_small(fb, 120, 154, "HA SYNCING...", C_TEXT_DIM, false);
        }
    }
}
