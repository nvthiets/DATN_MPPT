#ifndef SEVEN_SEGMENT_H
#define SEVEN_SEGMENT_H
#include "fonts.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Version 4: upright straight edges, no per-row shift or side ledges.
 * GFX font for ST7789_WriteGFXString.
 * Digits 0..9, '-' '.' ':'. No letters or UTF-8.
 * Digit bitmap: 28x48; xAdvance: 32; y is baseline (top=y-47).
 * Link seven_segment.c exactly once; do not also compile the same font
 * through an enabled Fonts/SevenSegment28x48.h definition in fonts.c. */
extern const GFXfont SevenSegment28x48;
#ifdef __cplusplus
}
#endif
#endif
