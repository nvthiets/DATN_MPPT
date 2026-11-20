#ifndef __FONT_H
#define __FONT_H

#include "stdint.h"

typedef struct {
    const uint8_t width;
    uint8_t height;
    const uint16_t *data;
} FontDef;

//Font lib.
extern FontDef Font_7x10;
extern FontDef Font_11x18;
extern FontDef Font_16x26;

/* Optional demo images: enable ST7789_ENABLE_DEMO_IMAGES in compiler defines
 * for the entire project, including fonts.c. Disabled by default (~369.5 KiB). */
#ifdef ST7789_ENABLE_DEMO_IMAGES
extern const uint16_t saber[][128];
extern const uint16_t knky[][240];
extern const uint16_t tek[][240];
extern const uint16_t adi1[][240];
#endif

/* Adafruit-compatible font types. */
#include <stdint.h>
/* STM32 memory-mapped const data: no AVR program-memory accessors needed. */
#ifndef PROGMEM
#define PROGMEM
#endif
/* Adafruit GFX font layout; pointers qualified const for read-only use. */
typedef struct {
    uint16_t bitmapOffset;
    uint8_t width, height, xAdvance;
    int8_t xOffset, yOffset;
} GFXglyph;
typedef struct {
    const uint8_t *bitmap;
    const GFXglyph *glyph;
    uint16_t first, last;
    uint8_t yAdvance;
} GFXfont;

#include "fonts_config.h"

#ifdef __cplusplus
extern "C" {
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeMono12pt7b
extern const GFXfont FreeMono12pt7b;
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeMono18pt7b
extern const GFXfont FreeMono18pt7b;
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeMono24pt7b
extern const GFXfont FreeMono24pt7b;
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeMono9pt7b
extern const GFXfont FreeMono9pt7b;
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeMonoBold12pt7b
extern const GFXfont FreeMonoBold12pt7b;
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeMonoBold18pt7b
extern const GFXfont FreeMonoBold18pt7b;
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeMonoBold24pt7b
extern const GFXfont FreeMonoBold24pt7b;
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeMonoBold9pt7b
extern const GFXfont FreeMonoBold9pt7b;
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeMonoBoldOblique12pt7b
extern const GFXfont FreeMonoBoldOblique12pt7b;
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeMonoBoldOblique18pt7b
extern const GFXfont FreeMonoBoldOblique18pt7b;
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeMonoBoldOblique24pt7b
extern const GFXfont FreeMonoBoldOblique24pt7b;
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeMonoBoldOblique9pt7b
extern const GFXfont FreeMonoBoldOblique9pt7b;
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeMonoOblique12pt7b
extern const GFXfont FreeMonoOblique12pt7b;
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeMonoOblique18pt7b
extern const GFXfont FreeMonoOblique18pt7b;
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeMonoOblique24pt7b
extern const GFXfont FreeMonoOblique24pt7b;
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeMonoOblique9pt7b
extern const GFXfont FreeMonoOblique9pt7b;
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeSans12pt7b
extern const GFXfont FreeSans12pt7b;
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeSans18pt7b
extern const GFXfont FreeSans18pt7b;
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeSans24pt7b
extern const GFXfont FreeSans24pt7b;
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeSans9pt7b
extern const GFXfont FreeSans9pt7b;
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeSansBold12pt7b
extern const GFXfont FreeSansBold12pt7b;
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeSansBold18pt7b
extern const GFXfont FreeSansBold18pt7b;
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeSansBold24pt7b
extern const GFXfont FreeSansBold24pt7b;
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeSansBold9pt7b
extern const GFXfont FreeSansBold9pt7b;
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeSansBoldOblique12pt7b
extern const GFXfont FreeSansBoldOblique12pt7b;
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeSansBoldOblique18pt7b
extern const GFXfont FreeSansBoldOblique18pt7b;
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeSansBoldOblique24pt7b
extern const GFXfont FreeSansBoldOblique24pt7b;
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeSansBoldOblique9pt7b
extern const GFXfont FreeSansBoldOblique9pt7b;
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeSansOblique12pt7b
extern const GFXfont FreeSansOblique12pt7b;
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeSansOblique18pt7b
extern const GFXfont FreeSansOblique18pt7b;
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeSansOblique24pt7b
extern const GFXfont FreeSansOblique24pt7b;
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeSansOblique9pt7b
extern const GFXfont FreeSansOblique9pt7b;
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeSerif12pt7b
extern const GFXfont FreeSerif12pt7b;
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeSerif18pt7b
extern const GFXfont FreeSerif18pt7b;
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeSerif24pt7b
extern const GFXfont FreeSerif24pt7b;
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeSerif9pt7b
extern const GFXfont FreeSerif9pt7b;
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeSerifBold12pt7b
extern const GFXfont FreeSerifBold12pt7b;
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeSerifBold18pt7b
extern const GFXfont FreeSerifBold18pt7b;
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeSerifBold24pt7b
extern const GFXfont FreeSerifBold24pt7b;
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeSerifBold9pt7b
extern const GFXfont FreeSerifBold9pt7b;
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeSerifBoldItalic12pt7b
extern const GFXfont FreeSerifBoldItalic12pt7b;
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeSerifBoldItalic18pt7b
extern const GFXfont FreeSerifBoldItalic18pt7b;
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeSerifBoldItalic24pt7b
extern const GFXfont FreeSerifBoldItalic24pt7b;
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeSerifBoldItalic9pt7b
extern const GFXfont FreeSerifBoldItalic9pt7b;
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeSerifItalic12pt7b
extern const GFXfont FreeSerifItalic12pt7b;
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeSerifItalic18pt7b
extern const GFXfont FreeSerifItalic18pt7b;
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeSerifItalic24pt7b
extern const GFXfont FreeSerifItalic24pt7b;
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeSerifItalic9pt7b
extern const GFXfont FreeSerifItalic9pt7b;
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_Org_01
extern const GFXfont Org_01;
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_Picopixel
extern const GFXfont Picopixel;
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_Tiny3x3a2pt7b
extern const GFXfont Tiny3x3a2pt7b;
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_TomThumb
extern const GFXfont TomThumb;
#endif
#ifdef __cplusplus
}
#endif
extern const GFXfont SevenSegment28x48;
#endif /* __FONT_H */
