#pragma once

#include <stdint.h>
#include "graphics.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CELEBRATION_COUNT 10

/**
 * @brief Render one of the 10 playful celebration animations directly into the framebuffer.
 * 
 * Animations:
 *  0: Superstar Clean Tooth (Smiling bobbing tooth, rainbow bezel, confetti, sparkle stars)
 *  1: Fireworks Spectacular (4 multi-stage bursting shells, cascading sparks, gravity fall)
 *  2: Space Rocket Blast-Off (Retro rocket ship, thruster fire plume, hyperspace star speed)
 *  3: Golden Trophy Champion (Sunburst rays, championship chalice cup with #1, golden rain)
 *  4: Rainbow Spiral Vortex (Hypnotic swirling 6-arm rainbow vortex, pulsing center star)
 *  5: Happy Smiling Emoji (Cheerful yellow smiley with bright pearly whites, party balloons)
 *  6: Cyber Disco Laser Party (Mirrored disco ball, 8 sweeping laser beams, neon equalizer)
 *  7: Bubble Pop Fiesta (Translucent soapy bubbles floating & popping into splash droplets)
 *  8: Superhero Lightning Shield (Concentric hero shield with star, crackling electric lightning)
 *  9: Carnival Confetti Cannon (Dual bottom cannons firing streamers, spirals, and balloons)
 * 
 * @param fb Framebuffer (240x240 RGB565)
 * @param anim_idx Animation index (0 to 9)
 * @param frame Monotonic animation frame counter (~30 FPS)
 */
/**
 * @brief Initialize celebration lookup tables (fast circle spans, etc.)
 */
void celebrations_init(void);

void render_celebration(uint16_t *fb, uint8_t anim_idx, uint32_t frame);

#ifdef __cplusplus
}
#endif
