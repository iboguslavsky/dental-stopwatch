#include "celebrations.h"
#include "angle_table.h"
#include "tooth_img.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define TFT_WIDTH  240
#define TFT_HEIGHT 240

// Rainbow palette for celebration animations
static const uint16_t C_RAINBOW[] = {
    COLOR_RED,
    COLOR_ORANGE,
    COLOR_YELLOW,
    COLOR_GREEN,
    COLOR_SKY_BLUE,
    COLOR_PINK
};

// Fast precomputed circular row half-spans (radius = 118)
static uint8_t s_circle_span[240];

void celebrations_init(void)
{
    for (int y = 0; y < 240; y++) {
        int dy = y - 120;
        int r2 = 118 * 118 - dy * dy;
        s_circle_span[y] = (r2 > 0) ? (uint8_t)sqrtf((float)r2) : 0;
    }
}

// ---------------------------------------------------------------------------
// Internal Geometric & Drawing Primitives (High Performance)
// ---------------------------------------------------------------------------

static void clear_circular_bg(uint16_t *fb, uint16_t bg_color)
{
    for (int y = 0; y < TFT_HEIGHT; y++) {
        int hw = s_circle_span[y];
        int row = y * TFT_WIDTH;
        if (hw == 0) {
            memset(&fb[row], 0, TFT_WIDTH * sizeof(uint16_t));
        } else {
            int x_start = 120 - hw;
            int x_end   = 120 + hw;
            if (x_start > 0) {
                memset(&fb[row], 0, x_start * sizeof(uint16_t));
            }
            for (int x = x_start; x <= x_end; x++) {
                fb[row + x] = bg_color;
            }
            if (x_end < TFT_WIDTH - 1) {
                memset(&fb[row + x_end + 1], 0, (TFT_WIDTH - 1 - x_end) * sizeof(uint16_t));
            }
        }
    }
}

static void draw_filled_rect(uint16_t *fb, int x, int y, int w, int h, uint16_t color)
{
    if (w <= 0 || h <= 0) return;
    int x1 = x, x2 = x + w - 1;
    int y1 = y, y2 = y + h - 1;

    if (x1 < 0) x1 = 0;
    if (y1 < 0) y1 = 0;
    if (x2 >= TFT_WIDTH)  x2 = TFT_WIDTH - 1;
    if (y2 >= TFT_HEIGHT) y2 = TFT_HEIGHT - 1;

    for (int py = y1; py <= y2; py++) {
        int row = py * TFT_WIDTH;
        for (int px = x1; px <= x2; px++) {
            fb[row + px] = color;
        }
    }
}

static void draw_filled_triangle(uint16_t *fb, int x0, int y0, int x1, int y1, int x2, int y2, uint16_t color)
{
    int min_x = x0 < x1 ? (x0 < x2 ? x0 : x2) : (x1 < x2 ? x1 : x2);
    int max_x = x0 > x1 ? (x0 > x2 ? x0 : x2) : (x1 > x2 ? x1 : x2);
    int min_y = y0 < y1 ? (y0 < y2 ? y0 : y2) : (y1 < y2 ? y1 : y2);
    int max_y = y0 > y1 ? (y0 > y2 ? y0 : y2) : (y1 > y2 ? y1 : y2);

    if (min_x < 0) min_x = 0;
    if (min_y < 0) min_y = 0;
    if (max_x >= TFT_WIDTH)  max_x = TFT_WIDTH - 1;
    if (max_y >= TFT_HEIGHT) max_y = TFT_HEIGHT - 1;

    for (int y = min_y; y <= max_y; y++) {
        int row = y * TFT_WIDTH;
        for (int x = min_x; x <= max_x; x++) {
            int w0 = (x1 - x0) * (y - y0) - (y1 - y0) * (x - x0);
            int w1 = (x2 - x1) * (y - y1) - (y2 - y1) * (x - x1);
            int w2 = (x0 - x2) * (y - y2) - (y0 - y2) * (x - x2);
            if ((w0 >= 0 && w1 >= 0 && w2 >= 0) || (w0 <= 0 && w1 <= 0 && w2 <= 0)) {
                fb[row + x] = color;
            }
        }
    }
}

static void draw_thick_line(uint16_t *fb, int x0, int y0, int x1, int y1, int width, uint16_t color)
{
    int half = width / 2;
    for (int ox = -half; ox <= half; ox++) {
        for (int oy = -half; oy <= half; oy++) {
            if (ox * ox + oy * oy <= half * half + 1) {
                draw_line(fb, x0 + ox, y0 + oy, x1 + ox, y1 + oy, color);
            }
        }
    }
}

// ---------------------------------------------------------------------------
// ANIMATION 0: Superstar Smiling Tooth
// ---------------------------------------------------------------------------
static void anim_superstar_tooth(uint16_t *fb, uint32_t frame)
{
    clear_circular_bg(fb, COLOR_DARK_BG);

    // Fast Bezel Ring (only calculate angle for r between 113 and 118)
    for (int y = 0; y < TFT_HEIGHT; y++) {
        int dy = y - 120;
        int ady = abs(dy);
        if (ady >= 119) continue;
        int hw = s_circle_span[y];
        if (hw < 113) continue;

        int row = y * TFT_WIDTH;
        // Inner circle cutoff
        int hw_inner2 = 113 * 113 - dy * dy;
        int hw_inner = (hw_inner2 > 0) ? (int)sqrtf((float)hw_inner2) : 0;

        // Left segment: x from (120 - hw) to (120 - hw_inner)
        for (int x = 120 - hw; x <= 120 - hw_inner; x++) {
            int dx = x - 120;
            int adx = abs(dx);
            if (adx >= 120) continue;
            uint16_t q = s_quadrant_angle_x10[ady][adx];
            uint16_t ang10 = (dy <= 0) ? (3600 - q) : (1800 + q);
            int color_idx = (((ang10 + (frame * 60)) % 3600) / 600) % 6;
            fb[row + x] = C_RAINBOW[color_idx];
        }

        // Right segment: x from (120 + hw_inner) to (120 + hw)
        for (int x = 120 + hw_inner; x <= 120 + hw; x++) {
            int dx = x - 120;
            int adx = abs(dx);
            if (adx >= 120) continue;
            uint16_t q = s_quadrant_angle_x10[ady][adx];
            uint16_t ang10 = (dy <= 0) ? q : (1800 - q);
            int color_idx = (((ang10 + (frame * 60)) % 3600) / 600) % 6;
            fb[row + x] = C_RAINBOW[color_idx];
        }
    }

    draw_confetti(fb, frame);

    int p1 = 3 + (int)(2.0f * sinf((float)frame * 0.3f));
    int p2 = 3 + (int)(2.0f * cosf((float)frame * 0.35f));
    draw_sparkle_star(fb, 45, 45, p1, COLOR_GOLD);
    draw_sparkle_star(fb, 195, 45, p2, COLOR_CYAN);
    draw_sparkle_star(fb, 45, 195, p2, COLOR_PINK);
    draw_sparkle_star(fb, 195, 195, p1, COLOR_GREEN);

    int bob_y = (int)(4.0f * sinf((float)frame * 0.25f));
    draw_smiling_tooth(fb, 120, 108 + bob_y);

    uint16_t top_col = ((frame / 8) % 2 == 0) ? COLOR_GOLD : COLOR_WHITE;
    draw_text_medium(fb, 120, 52, "SUPERSTAR!", top_col, true);
    draw_text_medium(fb, 120, 148, "100% CLEAN!", COLOR_GREEN, true);
    draw_text_small(fb, 120, 174, "SPARKLING TEETH!", COLOR_CYAN, true);
}

// ---------------------------------------------------------------------------
// ANIMATION 1: Fireworks Spectacular
// ---------------------------------------------------------------------------
static void anim_fireworks(uint16_t *fb, uint32_t frame)
{
    clear_circular_bg(fb, COLOR_DARK_BG);

    // Twinkling background stars
    static const int s_stars[8][2] = {
        {45, 60}, {190, 55}, {35, 150}, {205, 145},
        {120, 30}, {70, 180}, {170, 180}, {120, 210}
    };
    for (int i = 0; i < 8; i++) {
        int pulse = 2 + (int)(1.8f * sinf((float)(frame + i * 20) * 0.2f));
        draw_sparkle_star(fb, s_stars[i][0], s_stars[i][1], pulse, COLOR_WHITE);
    }

    // 4 Staggered Fireworks
    struct {
        int cx, cy;
        uint16_t color;
        int delay;
    } bursts[4] = {
        {75, 85,   COLOR_GOLD,     0},
        {165, 80,  COLOR_PINK,     18},
        {120, 110, COLOR_CYAN,     36},
        {90, 140,  COLOR_GREEN,    54}
    };

    for (int b = 0; b < 4; b++) {
        int f = (frame + bursts[b].delay) % 72;
        int cx = bursts[b].cx;
        int cy = bursts[b].cy;
        uint16_t col = bursts[b].color;

        if (f < 16) {
            // Rocket rising
            int ry = 220 - f * (220 - cy) / 16;
            draw_filled_circle(fb, cx, ry, 2, COLOR_WHITE);
            draw_line(fb, cx, ry + 1, cx, ry + 6, COLOR_ORANGE);
            draw_line(fb, cx - 1, ry + 3, cx + 1, ry + 3, COLOR_GOLD);
        } else {
            // Burst explosion!
            int t = f - 16;
            float radius = (float)t * 1.5f;
            float droop = (float)(t * t) * 0.035f;

            if (t < 5) {
                draw_filled_circle(fb, cx, cy, 7 - t, COLOR_WHITE);
            }

            for (int s = 0; s < 16; s++) {
                float ang = (float)s * (2.0f * (float)M_PI / 16.0f);
                int px = cx + (int)(radius * cosf(ang));
                int py = cy + (int)(radius * sinf(ang) + droop);

                if (px >= 2 && px < TFT_WIDTH - 2 && py >= 2 && py < TFT_HEIGHT - 2) {
                    uint16_t spark_col = (t % 3 == 0) ? COLOR_WHITE : col;
                    fb[py * TFT_WIDTH + px] = spark_col;
                    fb[py * TFT_WIDTH + px + 1] = spark_col;
                    fb[(py + 1) * TFT_WIDTH + px] = spark_col;
                }
            }
        }
    }

    uint16_t top_col = ((frame / 6) % 2 == 0) ? COLOR_GOLD : COLOR_WHITE;
    draw_text_medium(fb, 120, 42, "FANTASTIC!", top_col, true);
    draw_text_medium(fb, 120, 172, "SUPER BRUSH!", COLOR_SKY_BLUE, true);
    draw_text_small(fb, 120, 196, "FIREWORKS SHOW!", COLOR_PINK, true);
}

// ---------------------------------------------------------------------------
// ANIMATION 2: Space Rocket Blast-Off
// ---------------------------------------------------------------------------
static void anim_space_rocket(uint16_t *fb, uint32_t frame)
{
    clear_circular_bg(fb, COLOR_DARK_BG);

    // Hyperspace speed streaks streaming downwards
    for (int i = 0; i < 28; i++) {
        uint32_t seed = (uint32_t)i * 97 + 13;
        int sx = (int)(seed % 210) + 15;
        int speed = 3 + (int)(seed % 5);
        int sy = (int)((frame * speed + seed * 7) % 220) + 10;
        int len = 3 + speed * 2;
        draw_line(fb, sx, sy, sx, sy + len, (i % 2 == 0) ? COLOR_CYAN : COLOR_WHITE);
    }

    // Earth/Planet crescent glow at bottom
    draw_circle(fb, 120, 240, 120, COLOR_SKY_BLUE);
    draw_circle(fb, 120, 240, 121, COLOR_CYAN);

    // Rocket Bobbing Position
    int rx = 120 + (int)(3.0f * sinf((float)frame * 0.2f));
    int ry = 100;

    // Thruster Flames
    int fl_outer = 18 + (frame % 4) * 4;
    int fl_mid   = 12 + ((frame + 1) % 4) * 3;
    int fl_core  = 7  + ((frame + 2) % 4) * 2;

    draw_filled_triangle(fb, rx - 7, ry + 16, rx + 7, ry + 16, rx, ry + 16 + fl_outer, COLOR_RED);
    draw_filled_triangle(fb, rx - 5, ry + 16, rx + 5, ry + 16, rx, ry + 16 + fl_mid, COLOR_ORANGE);
    draw_filled_triangle(fb, rx - 3, ry + 16, rx + 3, ry + 16, rx, ry + 16 + fl_core, COLOR_WHITE);

    // Trailing sparks
    for (int p = 0; p < 6; p++) {
        int py = ry + 22 + (int)((frame * 4 + p * 12) % 40);
        int px = rx + (int)(sinf((float)(frame + p * 8) * 0.4f) * 6.0f);
        if (py < 235) {
            draw_filled_circle(fb, px, py, 1, (p % 2 == 0) ? COLOR_GOLD : COLOR_ORANGE);
        }
    }

    // Rocket Body (Fuselage)
    draw_filled_rect(fb, rx - 8, ry - 14, 17, 30, COLOR_WHITE);
    draw_line(fb, rx - 8, ry - 14, rx - 8, ry + 16, COLOR_RING);
    draw_line(fb, rx + 8, ry - 14, rx + 8, ry + 16, COLOR_RING);

    // Red Nose Cone
    draw_filled_triangle(fb, rx - 8, ry - 14, rx + 8, ry - 14, rx, ry - 32, COLOR_RED);

    // Left & Right Swept Wings
    draw_filled_triangle(fb, rx - 8, ry + 4, rx - 18, ry + 20, rx - 8, ry + 18, COLOR_RED);
    draw_filled_triangle(fb, rx + 8, ry + 4, rx + 18, ry + 20, rx + 8, ry + 18, COLOR_RED);

    // Round Porthole Window
    draw_filled_circle(fb, rx, ry - 1, 5, COLOR_SKY_BLUE);
    draw_circle(fb, rx, ry - 1, 5, COLOR_RING);
    draw_filled_circle(fb, rx - 1, ry - 3, 1, COLOR_WHITE);

    uint16_t top_col = ((frame / 6) % 2 == 0) ? COLOR_ORANGE : COLOR_GOLD;
    draw_text_medium(fb, 120, 42, "BLAST OFF!", top_col, true);
    draw_text_medium(fb, 120, 168, "STELLAR CLEAN!", COLOR_SKY_BLUE, true);
    draw_text_small(fb, 120, 192, "MISSION COMPLETE!", COLOR_WHITE, true);
}

// ---------------------------------------------------------------------------
// ANIMATION 3: Golden Trophy Champion
// ---------------------------------------------------------------------------
static void anim_trophy_champion(uint16_t *fb, uint32_t frame)
{
    // Fast 12-segment rotating sunburst rays
    for (int y = 0; y < TFT_HEIGHT; y++) {
        int dy = y - 120;
        int ady = abs(dy);
        int hw = s_circle_span[y];
        int row = y * TFT_WIDTH;

        if (hw == 0) {
            memset(&fb[row], 0, TFT_WIDTH * sizeof(uint16_t));
            continue;
        }

        int x_start = 120 - hw;
        int x_end   = 120 + hw;
        if (x_start > 0) memset(&fb[row], 0, x_start * sizeof(uint16_t));
        if (x_end < TFT_WIDTH - 1) memset(&fb[row + x_end + 1], 0, (TFT_WIDTH - 1 - x_end) * sizeof(uint16_t));

        for (int x = x_start; x <= x_end; x++) {
            int dx = x - 120;
            int adx = abs(dx);
            if (adx >= 120) continue;

            uint16_t q = s_quadrant_angle_x10[ady][adx];
            uint16_t ang10;
            if (dx >= 0 && dy <= 0)      ang10 = q;
            else if (dx >= 0 && dy > 0)  ang10 = 1800 - q;
            else if (dx < 0 && dy > 0)   ang10 = 1800 + q;
            else                         ang10 = 3600 - q;

            int ray_idx = ((ang10 + (frame * 12)) / 300) % 2;
            fb[row + x] = (ray_idx == 0) ? SWAP_U16(0x2124) : COLOR_DARK_BG;
        }
    }

    // Outer rim gold
    draw_circle(fb, 120, 120, 117, COLOR_GOLD);
    draw_circle(fb, 120, 120, 116, COLOR_GOLD);

    // Trophy Base (Pedestal)
    draw_filled_rect(fb, 98, 138, 45, 6, SWAP_U16(0x9400));
    draw_filled_rect(fb, 104, 132, 33, 6, COLOR_GOLD);
    draw_filled_rect(fb, 116, 120, 9, 12, COLOR_GOLD);      // Stem

    // Trophy Chalice Cup
    draw_filled_circle(fb, 120, 108, 18, COLOR_GOLD);
    draw_filled_rect(fb, 102, 94, 37, 14, COLOR_GOLD);
    draw_filled_rect(fb, 100, 91, 41, 4, COLOR_WHITE);       // Rim

    // Twin Cup Handles
    draw_circle(fb, 97, 104, 8, COLOR_GOLD);
    draw_circle(fb, 97, 104, 7, COLOR_GOLD);
    draw_circle(fb, 143, 104, 8, COLOR_GOLD);
    draw_circle(fb, 143, 104, 7, COLOR_GOLD);

    // "#1" Badge on cup
    draw_text_medium(fb, 120, 98, "#1", COLOR_WHITE, true);

    // Sparkling rim star
    int spark_sz = 3 + (frame % 3);
    draw_sparkle_star(fb, 140, 91, spark_sz, COLOR_WHITE);

    // Golden coin / star rain
    for (int i = 0; i < 14; i++) {
        uint32_t seed = (uint32_t)i * 109 + 23;
        int cx = 35 + (int)(seed % 170);
        int cy = (int)((frame * 2 + seed * 9) % 210) + 15;
        if (i % 2 == 0) {
            draw_filled_circle(fb, cx, cy, 2, COLOR_GOLD);
            draw_circle(fb, cx, cy, 2, COLOR_WHITE);
        } else {
            draw_sparkle_star(fb, cx, cy, 2, COLOR_YELLOW);
        }
    }

    uint16_t top_col = ((frame / 6) % 2 == 0) ? COLOR_GOLD : COLOR_WHITE;
    draw_text_medium(fb, 120, 42, "#1 CHAMPION!", top_col, true);
    draw_text_medium(fb, 120, 166, "BRUSHING MASTER!", COLOR_GREEN, true);
    draw_text_small(fb, 120, 190, "GOLD MEDAL WINNER!", COLOR_WHITE, true);
}

// ---------------------------------------------------------------------------
// ANIMATION 4: Rainbow Spiral Vortex (ULTRA FAST - NO SQRT, NO FLOATS)
// ---------------------------------------------------------------------------
static void anim_rainbow_vortex(uint16_t *fb, uint32_t frame)
{
    // Fast integer-only swirling rainbow galaxy
    for (int y = 0; y < TFT_HEIGHT; y++) {
        int dy = y - 120;
        int ady = abs(dy);
        int hw = s_circle_span[y];
        int row = y * TFT_WIDTH;

        if (hw == 0) {
            memset(&fb[row], 0, TFT_WIDTH * sizeof(uint16_t));
            continue;
        }

        int x_start = 120 - hw;
        int x_end   = 120 + hw;
        if (x_start > 0) memset(&fb[row], 0, x_start * sizeof(uint16_t));
        if (x_end < TFT_WIDTH - 1) memset(&fb[row + x_end + 1], 0, (TFT_WIDTH - 1 - x_end) * sizeof(uint16_t));

        for (int x = x_start; x <= x_end; x++) {
            int dx = x - 120;
            int adx = abs(dx);
            if (adx >= 120) continue;

            // Fast integer quadrant angle from flash table
            uint16_t q = s_quadrant_angle_x10[ady][adx];
            uint16_t ang10;
            if (dx >= 0 && dy <= 0)      ang10 = q;
            else if (dx >= 0 && dy > 0)  ang10 = 1800 - q;
            else if (dx < 0 && dy > 0)   ang10 = 1800 + q;
            else                         ang10 = 3600 - q;

            // Fast integer radius approximation: max + 3/8 min (accurate to 3%, 0 cycles float!)
            int r_int = (adx > ady) ? (adx + ((ady * 3) >> 3)) : (ady + ((adx * 3) >> 3));

            // Fast integer spiral formula
            uint32_t spiral = (uint32_t)ang10 + (uint32_t)(r_int * 24) + (uint32_t)(frame * 65);
            int col_idx = ((spiral % 3600) / 600) % 6;
            fb[row + x] = C_RAINBOW[col_idx];
        }
    }

    // Outer rim gold
    draw_circle(fb, 120, 120, 117, COLOR_GOLD);
    draw_circle(fb, 120, 120, 116, COLOR_GOLD);

    // Concentric white energy shockwaves rippling out from center
    int r1 = (frame * 2) % 85;
    int r2 = ((frame * 2) + 42) % 85;
    if (r1 > 4) draw_circle(fb, 120, 120, r1, COLOR_WHITE);
    if (r2 > 4) draw_circle(fb, 120, 120, r2, COLOR_WHITE);

    // Pulsing central star emblem
    int star_sz = 6 + (int)(3.0f * sinf((float)frame * 0.3f));
    draw_sparkle_star(fb, 120, 120, star_sz, COLOR_WHITE);
    draw_sparkle_star(fb, 120, 120, star_sz - 2, COLOR_GOLD);

    draw_text_medium(fb, 120, 48, "WOW! AMAZING!", COLOR_WHITE, true);
    draw_text_medium(fb, 120, 148, "CLEAN TEETH POWER!", COLOR_GOLD, true);
    draw_text_small(fb, 120, 174, "SUPER GROOVY!", COLOR_CYAN, true);
}

// ---------------------------------------------------------------------------
// ANIMATION 5: Happy Smiling Emoji
// ---------------------------------------------------------------------------
static void anim_smiling_emoji(uint16_t *fb, uint32_t frame)
{
    clear_circular_bg(fb, COLOR_DARK_BG);

    // Floating party balloons drifting upwards
    static const struct {
        int x;
        uint16_t color;
        int speed;
    } s_balloons[5] = {
        {40,  COLOR_RED,      2},
        {70,  COLOR_SKY_BLUE, 3},
        {120, COLOR_GREEN,    2},
        {170, COLOR_PINK,     3},
        {200, COLOR_YELLOW,   2}
    };

    for (int b = 0; b < 5; b++) {
        int by = (int)(240 - ((frame * s_balloons[b].speed + b * 55) % 270)) - 10;
        int bx = s_balloons[b].x + (int)(4.0f * sinf((float)(frame + b * 15) * 0.15f));
        if (by > -20 && by < 240) {
            draw_filled_circle(fb, bx, by, 8, s_balloons[b].color);
            draw_line(fb, bx, by + 8, bx, by + 18, COLOR_RING);
        }
    }

    // Center Giant Yellow Smiley Face (radius 32)
    int em_x = 120;
    int em_y = 110 + (int)(2.5f * sinf((float)frame * 0.2f));

    draw_filled_circle(fb, em_x, em_y, 32, COLOR_YELLOW);
    draw_circle(fb, em_x, em_y, 32, COLOR_BLACK);
    draw_circle(fb, em_x, em_y, 31, COLOR_BLACK);

    // Rosy Pink Cheeks
    draw_filled_circle(fb, em_x - 20, em_y + 8, 4, COLOR_PINK);
    draw_filled_circle(fb, em_x + 20, em_y + 8, 4, COLOR_PINK);

    // Left Eye: Playful Wink '^'
    draw_line(fb, em_x - 18, em_y - 4, em_x - 12, em_y - 10, COLOR_BLACK);
    draw_line(fb, em_x - 12, em_y - 10, em_x - 6, em_y - 4, COLOR_BLACK);
    draw_line(fb, em_x - 18, em_y - 3, em_x - 12, em_y - 9, COLOR_BLACK);
    draw_line(fb, em_x - 12, em_y - 9, em_x - 6, em_y - 3, COLOR_BLACK);

    // Right Eye: Joyful Wide Open Eye with white glint
    draw_filled_circle(fb, em_x + 12, em_y - 6, 5, COLOR_BLACK);
    draw_filled_circle(fb, em_x + 10, em_y - 8, 2, COLOR_WHITE);

    // Big Smiling Mouth with Pearly-White Clean Teeth!
    draw_filled_circle(fb, em_x, em_y + 10, 14, COLOR_BLACK);
    draw_filled_rect(fb, em_x - 15, em_y - 2, 31, 12, COLOR_YELLOW);

    // Row of bright white teeth along top edge of smile
    draw_filled_rect(fb, em_x - 9, em_y + 10, 19, 5, COLOR_WHITE);
    draw_line(fb, em_x, em_y + 10, em_x, em_y + 14, COLOR_RING);

    // Sparkle star on tooth!
    draw_sparkle_star(fb, em_x + 5, em_y + 12, 3 + (frame % 3), COLOR_WHITE);

    uint16_t top_col = ((frame / 6) % 2 == 0) ? COLOR_GOLD : COLOR_WHITE;
    draw_text_medium(fb, 120, 42, "SO SHINY!", top_col, true);
    draw_text_medium(fb, 120, 164, "SMILE BRIGHT!", COLOR_GREEN, true);
    draw_text_small(fb, 120, 188, "GREAT JOB!", COLOR_SKY_BLUE, true);
}

// ---------------------------------------------------------------------------
// ANIMATION 6: Cyber Disco Laser Party
// ---------------------------------------------------------------------------
static void anim_disco_party(uint16_t *fb, uint32_t frame)
{
    clear_circular_bg(fb, COLOR_BLACK);

    // Disco Ball at top center
    int db_x = 120, db_y = 42, db_r = 17;
    draw_line(fb, db_x, 0, db_x, db_y - db_r, COLOR_WHITE);

    // 8 Sweeping Neon Laser Beams radiating across screen
    float t = (float)frame * 0.08f;
    int targets_x[8] = {
        30  + (int)(30.0f * sinf(t)),
        60  + (int)(40.0f * cosf(t * 1.1f)),
        90  + (int)(35.0f * sinf(t * 0.9f)),
        120 + (int)(50.0f * cosf(t * 1.2f)),
        150 + (int)(35.0f * sinf(t * 1.1f)),
        180 + (int)(40.0f * cosf(t * 0.95f)),
        210 + (int)(30.0f * sinf(t * 1.3f)),
        120 + (int)(80.0f * sinf(t * 0.7f))
    };
    uint16_t laser_cols[8] = {
        COLOR_PINK, COLOR_CYAN, COLOR_GREEN, COLOR_YELLOW,
        COLOR_PINK, COLOR_CYAN, COLOR_GREEN, COLOR_ORANGE
    };
    for (int i = 0; i < 8; i++) {
        draw_line(fb, db_x, db_y, targets_x[i], 220, laser_cols[i]);
    }

    // Mirrored Disco Ball facets
    draw_filled_circle(fb, db_x, db_y, db_r, SWAP_U16(0x52AA));
    for (int fy = -db_r + 3; fy < db_r - 2; fy += 4) {
        for (int fx = -db_r + 3; fx < db_r - 2; fx += 5) {
            if (fx * fx + fy * fy <= (db_r - 2) * (db_r - 2)) {
                uint16_t tile_col = ((fx + fy + (int)(frame * 2)) % 3 == 0) ? COLOR_WHITE : COLOR_CYAN;
                draw_filled_rect(fb, db_x + fx, db_y + fy, 3, 3, tile_col);
            }
        }
    }
    draw_circle(fb, db_x, db_y, db_r, COLOR_WHITE);

    // Neon Equalizer Bars at bottom
    for (int b = 0; b < 10; b++) {
        int bx = 50 + b * 15;
        int bh = 6 + (int)(18.0f * fabsf(sinf((float)(frame + b * 6) * 0.25f)));
        uint16_t bcol = C_RAINBOW[b % 6];
        draw_filled_rect(fb, bx, 185 - bh, 10, bh, bcol);
    }

    uint16_t top_col = ((frame / 6) % 2 == 0) ? COLOR_PINK : COLOR_CYAN;
    draw_text_medium(fb, 120, 68, "DISCO FEVER!", top_col, true);
    draw_text_medium(fb, 120, 142, "PARTY TIME!", COLOR_YELLOW, true);
    draw_text_small(fb, 120, 198, "TEETH ARE DANCING!", COLOR_GREEN, true);
}

// ---------------------------------------------------------------------------
// ANIMATION 7: Bubble Pop Fiesta
// ---------------------------------------------------------------------------
static void anim_bubble_pop(uint16_t *fb, uint32_t frame)
{
    clear_circular_bg(fb, COLOR_DARK_BG);

    // Shimmering soap bubbles floating upward
    struct {
        int x0, r, speed, phase;
    } s_bubbles[10] = {
        {45,  12, 2, 0},
        {75,  8,  3, 40},
        {105, 14, 2, 80},
        {135, 10, 3, 120},
        {165, 15, 2, 160},
        {195, 9,  3, 200},
        {60,  16, 2, 240},
        {120, 11, 3, 280},
        {180, 13, 2, 320},
        {150, 7,  3, 360}
    };

    for (int i = 0; i < 10; i++) {
        int by = 240 - ((frame * s_bubbles[i].speed + s_bubbles[i].phase) % 260);
        int bx = s_bubbles[i].x0 + (int)(5.0f * sinf((float)(frame + i * 15) * 0.12f));
        int r  = s_bubbles[i].r;

        if (by > 40 && by < 230) {
            draw_circle(fb, bx, by, r, COLOR_CYAN);
            draw_circle(fb, bx, by, r - 1, COLOR_SKY_BLUE);
            draw_line(fb, bx - r/2, by - r/2, bx - r/3, by - r/2, COLOR_WHITE);
            draw_line(fb, bx - r/2, by - r/2, bx - r/2, by - r/3, COLOR_WHITE);
        } else if (by <= 40 && by > 20) {
            int pop_t = 40 - by;
            for (int d = 0; d < 6; d++) {
                float ang = (float)d * (2.0f * (float)M_PI / 6.0f);
                int dx = bx + (int)((r + pop_t * 2) * cosf(ang));
                int dy = by + (int)((r + pop_t * 2) * sinf(ang));
                draw_filled_circle(fb, dx, dy, 1, COLOR_CYAN);
            }
        }
    }

    int cb_x = 120, cb_y = 112;
    draw_smiling_tooth(fb, cb_x, cb_y + (int)(2.0f * sinf((float)frame * 0.2f)));
    draw_circle(fb, cb_x, cb_y, 32, COLOR_CYAN);
    draw_circle(fb, cb_x, cb_y, 33, COLOR_SKY_BLUE);
    draw_sparkle_star(fb, cb_x - 18, cb_y - 18, 3, COLOR_WHITE);

    uint16_t top_col = ((frame / 6) % 2 == 0) ? COLOR_CYAN : COLOR_WHITE;
    draw_text_medium(fb, 120, 44, "BUBBLE POP!", top_col, true);
    draw_text_medium(fb, 120, 166, "SQUEAKY CLEAN!", COLOR_GREEN, true);
    draw_text_small(fb, 120, 190, "FRESH & SHINY!", COLOR_YELLOW, true);
}

// ---------------------------------------------------------------------------
// ANIMATION 8: Superhero Lightning Shield
// ---------------------------------------------------------------------------
static void anim_superhero_shield(uint16_t *fb, uint32_t frame)
{
    clear_circular_bg(fb, COLOR_DARK_BG);

    int cx = 120, cy = 110;

    // Outer Red Ring
    for (int r = 40; r <= 46; r++) {
        draw_circle(fb, cx, cy, r, COLOR_RED);
    }
    // Silver/White Ring
    for (int r = 30; r < 40; r++) {
        draw_circle(fb, cx, cy, r, COLOR_WHITE);
    }
    // Inner Blue Disc
    draw_filled_circle(fb, cx, cy, 29, COLOR_SKY_BLUE);

    // Center Golden Hero Star
    int s_sz = 14 + (int)(2.0f * sinf((float)frame * 0.35f));
    draw_sparkle_star(fb, cx, cy, s_sz, COLOR_GOLD);
    draw_sparkle_star(fb, cx, cy, s_sz / 2, COLOR_WHITE);

    // Procedural Electric Lightning Bolts arcing outwards
    int targets[4][2] = {
        {195, 45}, {195, 190}, {45, 190}, {45, 45}
    };
    for (int b = 0; b < 4; b++) {
        int x_cur = cx + (targets[b][0] - cx) / 3;
        int y_cur = cy + (targets[b][1] - cy) / 3;
        int x_end = targets[b][0];
        int y_end = targets[b][1];

        int steps = 5;
        for (int s = 0; s < steps; s++) {
            int x_next = x_cur + (x_end - x_cur) / (steps - s);
            int y_next = y_cur + (y_end - y_cur) / (steps - s);

            int jitter_seed = (int)frame * 7 + b * 19 + s * 11;
            int jx = ((jitter_seed % 11) - 5);
            int jy = (((jitter_seed * 3) % 11) - 5);

            if (s < steps - 1) {
                x_next += jx;
                y_next += jy;
            }

            draw_line(fb, x_cur, y_cur, x_next, y_next, COLOR_CYAN);
            draw_line(fb, x_cur + 1, y_cur, x_next + 1, y_next, COLOR_WHITE);
            x_cur = x_next;
            y_cur = y_next;
        }
    }

    uint16_t top_col = ((frame / 6) % 2 == 0) ? COLOR_GOLD : COLOR_WHITE;
    draw_text_medium(fb, 120, 42, "TOOTH HERO!", top_col, true);
    draw_text_medium(fb, 120, 166, "PLAQUE DEFEATED!", COLOR_ORANGE, true);
    draw_text_small(fb, 120, 190, "SUPER POWER CLEAN!", COLOR_CYAN, true);
}

// ---------------------------------------------------------------------------
// ANIMATION 9: Carnival Confetti Cannon
// ---------------------------------------------------------------------------
static void anim_carnival_cannons(uint16_t *fb, uint32_t frame)
{
    clear_circular_bg(fb, COLOR_DARK_BG);

    // Left Cannon at (45, 195) angled 45 deg right
    draw_thick_line(fb, 35, 205, 55, 185, 9, COLOR_RED);
    draw_thick_line(fb, 37, 203, 57, 183, 3, COLOR_GOLD);
    draw_filled_circle(fb, 36, 204, 7, SWAP_U16(0x52AA));

    // Right Cannon at (195, 195) angled 45 deg left
    draw_thick_line(fb, 205, 205, 185, 185, 9, COLOR_RED);
    draw_thick_line(fb, 203, 203, 183, 183, 3, COLOR_GOLD);
    draw_filled_circle(fb, 204, 204, 7, SWAP_U16(0x52AA));

    // Erupting Ballistic Confetti & Streamers
    for (int i = 0; i < 30; i++) {
        uint32_t seed = (uint32_t)i * 127 + 17;
        bool is_left = (i % 2 == 0);
        int origin_x = is_left ? 55 : 185;
        int origin_y = 185;

        int cycle = (int)((frame * 2 + seed) % 65);
        float t = (float)cycle * 0.12f;

        float vx = (is_left ? 1.0f : -1.0f) * (18.0f + (float)(seed % 10));
        float vy = -35.0f - (float)(seed % 14);
        float g  = 28.0f;

        int px = origin_x + (int)(vx * t);
        int py = origin_y + (int)(vy * t + 0.5f * g * t * t);

        if (px >= 4 && px < TFT_WIDTH - 4 && py >= 4 && py < TFT_HEIGHT - 4) {
            uint16_t col = C_RAINBOW[(i + frame / 6) % 6];
            fb[py * TFT_WIDTH + px] = col;
            fb[py * TFT_WIDTH + px + 1] = col;
            fb[(py + 1) * TFT_WIDTH + px] = col;
            fb[(py + 1) * TFT_WIDTH + px + 1] = col;
        }
    }

    int puff_l = 4 + (frame % 4);
    int puff_r = 4 + ((frame + 2) % 4);
    draw_circle(fb, 55, 185, puff_l, COLOR_WHITE);
    draw_circle(fb, 185, 185, puff_r, COLOR_WHITE);

    int bal_y = 220 - ((frame * 2) % 240);
    if (bal_y > 20 && bal_y < 220) {
        draw_filled_circle(fb, 120, bal_y, 10, COLOR_PINK);
        draw_line(fb, 120, bal_y + 10, 120, bal_y + 24, COLOR_RING);
    }

    uint16_t top_col = ((frame / 6) % 2 == 0) ? COLOR_GOLD : COLOR_WHITE;
    draw_text_medium(fb, 120, 44, "YOU DID IT!", top_col, true);
    draw_text_medium(fb, 120, 108, "ALL 2 MINUTES!", COLOR_YELLOW, true);
    draw_text_small(fb, 120, 148, "CARNIVAL WINNER!", COLOR_PINK, true);
}

// ---------------------------------------------------------------------------
// Celebration Dispatcher
// ---------------------------------------------------------------------------
void render_celebration(uint16_t *fb, uint8_t anim_idx, uint32_t frame)
{
    switch (anim_idx % CELEBRATION_COUNT) {
        case 0:
            anim_superstar_tooth(fb, frame);
            break;
        case 1:
            anim_fireworks(fb, frame);
            break;
        case 2:
            anim_space_rocket(fb, frame);
            break;
        case 3:
            anim_trophy_champion(fb, frame);
            break;
        case 4:
            anim_rainbow_vortex(fb, frame);
            break;
        case 5:
            anim_smiling_emoji(fb, frame);
            break;
        case 6:
            anim_disco_party(fb, frame);
            break;
        case 7:
            anim_bubble_pop(fb, frame);
            break;
        case 8:
            anim_superhero_shield(fb, frame);
            break;
        case 9:
            anim_carnival_cannons(fb, frame);
            break;
        default:
            anim_superstar_tooth(fb, frame);
            break;
    }
}
