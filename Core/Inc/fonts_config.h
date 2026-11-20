#ifndef ST7789_FONTS_CONFIG_H
#define ST7789_FONTS_CONFIG_H

/* Step 1 uses group 14: no optional preview fonts. UI bitmap fonts remain.
 * Preview selection: 0 = all 52 fonts (~215.3 KiB font data).
 * On smaller MCUs select group 1..13 (four fonts per build).
 * Group ordering is listed in README.md. */
#ifndef ST7789_FONT_PREVIEW_GROUP
#define ST7789_FONT_PREVIEW_GROUP 14
#endif
#if ST7789_FONT_PREVIEW_GROUP < 0 || ST7789_FONT_PREVIEW_GROUP > 14
#error ST7789_FONT_PREVIEW_GROUP must be 0..14
#endif

/* Set individual fonts to 1 to include their data in Flash. */
#ifndef ST7789_ENABLE_ALL_GFX_FONTS
#define ST7789_ENABLE_ALL_GFX_FONTS (ST7789_FONT_PREVIEW_GROUP == 0)
#endif
#ifndef ST7789_USE_FreeMono12pt7b
#define ST7789_USE_FreeMono12pt7b (ST7789_FONT_PREVIEW_GROUP == 1)
#endif
#ifndef ST7789_USE_FreeMono18pt7b
#define ST7789_USE_FreeMono18pt7b (ST7789_FONT_PREVIEW_GROUP == 1)
#endif
#ifndef ST7789_USE_FreeMono24pt7b
#define ST7789_USE_FreeMono24pt7b (ST7789_FONT_PREVIEW_GROUP == 1)
#endif
#ifndef ST7789_USE_FreeMono9pt7b
#define ST7789_USE_FreeMono9pt7b (ST7789_FONT_PREVIEW_GROUP == 1)
#endif
#ifndef ST7789_USE_FreeMonoBold12pt7b
#define ST7789_USE_FreeMonoBold12pt7b (ST7789_FONT_PREVIEW_GROUP == 2)
#endif
#ifndef ST7789_USE_FreeMonoBold18pt7b
#define ST7789_USE_FreeMonoBold18pt7b (ST7789_FONT_PREVIEW_GROUP == 2)
#endif
#ifndef ST7789_USE_FreeMonoBold24pt7b
#define ST7789_USE_FreeMonoBold24pt7b (ST7789_FONT_PREVIEW_GROUP == 2)
#endif
#ifndef ST7789_USE_FreeMonoBold9pt7b
#define ST7789_USE_FreeMonoBold9pt7b (ST7789_FONT_PREVIEW_GROUP == 2)
#endif
#ifndef ST7789_USE_FreeMonoBoldOblique12pt7b
#define ST7789_USE_FreeMonoBoldOblique12pt7b (ST7789_FONT_PREVIEW_GROUP == 4)
#endif
#ifndef ST7789_USE_FreeMonoBoldOblique18pt7b
#define ST7789_USE_FreeMonoBoldOblique18pt7b (ST7789_FONT_PREVIEW_GROUP == 4)
#endif
#ifndef ST7789_USE_FreeMonoBoldOblique24pt7b
#define ST7789_USE_FreeMonoBoldOblique24pt7b (ST7789_FONT_PREVIEW_GROUP == 4)
#endif
#ifndef ST7789_USE_FreeMonoBoldOblique9pt7b
#define ST7789_USE_FreeMonoBoldOblique9pt7b (ST7789_FONT_PREVIEW_GROUP == 4)
#endif
#ifndef ST7789_USE_FreeMonoOblique12pt7b
#define ST7789_USE_FreeMonoOblique12pt7b (ST7789_FONT_PREVIEW_GROUP == 3)
#endif
#ifndef ST7789_USE_FreeMonoOblique18pt7b
#define ST7789_USE_FreeMonoOblique18pt7b (ST7789_FONT_PREVIEW_GROUP == 3)
#endif
#ifndef ST7789_USE_FreeMonoOblique24pt7b
#define ST7789_USE_FreeMonoOblique24pt7b (ST7789_FONT_PREVIEW_GROUP == 3)
#endif
#ifndef ST7789_USE_FreeMonoOblique9pt7b
#define ST7789_USE_FreeMonoOblique9pt7b (ST7789_FONT_PREVIEW_GROUP == 3)
#endif
#ifndef ST7789_USE_FreeSans12pt7b
#define ST7789_USE_FreeSans12pt7b (ST7789_FONT_PREVIEW_GROUP == 5)
#endif
#ifndef ST7789_USE_FreeSans18pt7b
#define ST7789_USE_FreeSans18pt7b (ST7789_FONT_PREVIEW_GROUP == 5)
#endif
#ifndef ST7789_USE_FreeSans24pt7b
#define ST7789_USE_FreeSans24pt7b (ST7789_FONT_PREVIEW_GROUP == 5)
#endif
#ifndef ST7789_USE_FreeSans9pt7b
#define ST7789_USE_FreeSans9pt7b (ST7789_FONT_PREVIEW_GROUP == 5)
#endif
#ifndef ST7789_USE_FreeSansBold12pt7b
#define ST7789_USE_FreeSansBold12pt7b (ST7789_FONT_PREVIEW_GROUP == 6)
#endif
#ifndef ST7789_USE_FreeSansBold18pt7b
#define ST7789_USE_FreeSansBold18pt7b (ST7789_FONT_PREVIEW_GROUP == 6)
#endif
#ifndef ST7789_USE_FreeSansBold24pt7b
#define ST7789_USE_FreeSansBold24pt7b (ST7789_FONT_PREVIEW_GROUP == 6)
#endif
#ifndef ST7789_USE_FreeSansBold9pt7b
#define ST7789_USE_FreeSansBold9pt7b (ST7789_FONT_PREVIEW_GROUP == 6)
#endif
#ifndef ST7789_USE_FreeSansBoldOblique12pt7b
#define ST7789_USE_FreeSansBoldOblique12pt7b (ST7789_FONT_PREVIEW_GROUP == 8)
#endif
#ifndef ST7789_USE_FreeSansBoldOblique18pt7b
#define ST7789_USE_FreeSansBoldOblique18pt7b (ST7789_FONT_PREVIEW_GROUP == 8)
#endif
#ifndef ST7789_USE_FreeSansBoldOblique24pt7b
#define ST7789_USE_FreeSansBoldOblique24pt7b (ST7789_FONT_PREVIEW_GROUP == 8)
#endif
#ifndef ST7789_USE_FreeSansBoldOblique9pt7b
#define ST7789_USE_FreeSansBoldOblique9pt7b (ST7789_FONT_PREVIEW_GROUP == 8)
#endif
#ifndef ST7789_USE_FreeSansOblique12pt7b
#define ST7789_USE_FreeSansOblique12pt7b (ST7789_FONT_PREVIEW_GROUP == 7)
#endif
#ifndef ST7789_USE_FreeSansOblique18pt7b
#define ST7789_USE_FreeSansOblique18pt7b (ST7789_FONT_PREVIEW_GROUP == 7)
#endif
#ifndef ST7789_USE_FreeSansOblique24pt7b
#define ST7789_USE_FreeSansOblique24pt7b (ST7789_FONT_PREVIEW_GROUP == 7)
#endif
#ifndef ST7789_USE_FreeSansOblique9pt7b
#define ST7789_USE_FreeSansOblique9pt7b (ST7789_FONT_PREVIEW_GROUP == 7)
#endif
#ifndef ST7789_USE_FreeSerif12pt7b
#define ST7789_USE_FreeSerif12pt7b (ST7789_FONT_PREVIEW_GROUP == 9)
#endif
#ifndef ST7789_USE_FreeSerif18pt7b
#define ST7789_USE_FreeSerif18pt7b (ST7789_FONT_PREVIEW_GROUP == 9)
#endif
#ifndef ST7789_USE_FreeSerif24pt7b
#define ST7789_USE_FreeSerif24pt7b (ST7789_FONT_PREVIEW_GROUP == 9)
#endif
#ifndef ST7789_USE_FreeSerif9pt7b
#define ST7789_USE_FreeSerif9pt7b (ST7789_FONT_PREVIEW_GROUP == 9)
#endif
#ifndef ST7789_USE_FreeSerifBold12pt7b
#define ST7789_USE_FreeSerifBold12pt7b (ST7789_FONT_PREVIEW_GROUP == 10)
#endif
#ifndef ST7789_USE_FreeSerifBold18pt7b
#define ST7789_USE_FreeSerifBold18pt7b (ST7789_FONT_PREVIEW_GROUP == 10)
#endif
#ifndef ST7789_USE_FreeSerifBold24pt7b
#define ST7789_USE_FreeSerifBold24pt7b (ST7789_FONT_PREVIEW_GROUP == 10)
#endif
#ifndef ST7789_USE_FreeSerifBold9pt7b
#define ST7789_USE_FreeSerifBold9pt7b (ST7789_FONT_PREVIEW_GROUP == 10)
#endif
#ifndef ST7789_USE_FreeSerifBoldItalic12pt7b
#define ST7789_USE_FreeSerifBoldItalic12pt7b (ST7789_FONT_PREVIEW_GROUP == 12)
#endif
#ifndef ST7789_USE_FreeSerifBoldItalic18pt7b
#define ST7789_USE_FreeSerifBoldItalic18pt7b (ST7789_FONT_PREVIEW_GROUP == 12)
#endif
#ifndef ST7789_USE_FreeSerifBoldItalic24pt7b
#define ST7789_USE_FreeSerifBoldItalic24pt7b (ST7789_FONT_PREVIEW_GROUP == 12)
#endif
#ifndef ST7789_USE_FreeSerifBoldItalic9pt7b
#define ST7789_USE_FreeSerifBoldItalic9pt7b (ST7789_FONT_PREVIEW_GROUP == 12)
#endif
#ifndef ST7789_USE_FreeSerifItalic12pt7b
#define ST7789_USE_FreeSerifItalic12pt7b (ST7789_FONT_PREVIEW_GROUP == 11)
#endif
#ifndef ST7789_USE_FreeSerifItalic18pt7b
#define ST7789_USE_FreeSerifItalic18pt7b (ST7789_FONT_PREVIEW_GROUP == 11)
#endif
#ifndef ST7789_USE_FreeSerifItalic24pt7b
#define ST7789_USE_FreeSerifItalic24pt7b (ST7789_FONT_PREVIEW_GROUP == 11)
#endif
#ifndef ST7789_USE_FreeSerifItalic9pt7b
#define ST7789_USE_FreeSerifItalic9pt7b (ST7789_FONT_PREVIEW_GROUP == 11)
#endif
#ifndef ST7789_USE_Org_01
#define ST7789_USE_Org_01 (ST7789_FONT_PREVIEW_GROUP == 13)
#endif
#ifndef ST7789_USE_Picopixel
#define ST7789_USE_Picopixel (ST7789_FONT_PREVIEW_GROUP == 13)
#endif
#ifndef ST7789_USE_Tiny3x3a2pt7b
#define ST7789_USE_Tiny3x3a2pt7b (ST7789_FONT_PREVIEW_GROUP == 13)
#endif
#ifndef ST7789_USE_TomThumb
#define ST7789_USE_TomThumb (ST7789_FONT_PREVIEW_GROUP == 13)
#endif


#endif /* ST7789_FONTS_CONFIG_H */
