#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint8_t width;
    uint8_t height;
    uint8_t stride; // bytes per row
    const uint8_t *bitmap;
} glyph_info_t;

const glyph_info_t *ubuntu_large_get_glyph(char c);
const glyph_info_t *ubuntu_medium_get_glyph(char c);
const glyph_info_t *ubuntu_small_get_glyph(char c);
const glyph_info_t *ubuntu_temp_get_glyph(char c);

