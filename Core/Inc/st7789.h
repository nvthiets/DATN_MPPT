//nvthiets
#ifndef __ST7789_H
#define __ST7789_H

#include "fonts.h"
#include "main.h"

/* choose a Hardware SPI port to use. */
#define ST7789_SPI_PORT hspi1
extern SPI_HandleTypeDef ST7789_SPI_PORT;

/* choose whether use DMA or not */
//#define USE_DMA

/* If u need CS control, comment below*/
//#define CFG_NO_CS
// Schematic: CS=PA4 RST=PA1 DC=PA0 MOSI=PA7 SCK=PA5.
// If the physical wiring follows the old test, swap RST_PIN and DC_PIN below.
/* Pin connection*/
#define ST7789_RST_PORT  TFT_RST_GPIO_Port
#define ST7789_RST_PIN   TFT_RST_Pin

#define ST7789_DC_PORT   TFT_DC_GPIO_Port
#define ST7789_DC_PIN    TFT_DC_Pin

#ifndef CFG_NO_CS
#define ST7789_CS_PORT   TFT_CS_GPIO_Port
#define ST7789_CS_PIN    TFT_CS_Pin
#endif

/* If u need Backlight control, uncomment below */
//#define BLK_PORT
//#define BLK_PIN


/* Select exactly one native panel size. */
//#define USING_135X240
//#define USING_240X240
//#define USING_170X320
#define USING_240X320

#ifndef ST7789_ROTATION
#define ST7789_ROTATION 1
#endif
#if ST7789_ROTATION < 0 || ST7789_ROTATION > 3
#error ST7789_ROTATION must be 0..3
#endif
#if (defined(USING_135X240) + defined(USING_240X240) + defined(USING_170X320) + defined(USING_240X320)) != 1
#error Select exactly one panel size
#endif

/* Runtime dimensions: updated by ST7789_SetRotation().
 * These are no longer compile-time constants; use fixed-size local buffers. */
extern uint16_t ST7789_Width, ST7789_Height;
#define ST7789_WIDTH  ST7789_Width
#define ST7789_HEIGHT ST7789_Height

/* Default preserves the byte-swapped image arrays in the original fonts.c.
 * Set to 0 for ordinary uint16_t RGB565 arrays (RED = 0xF800).
 * ST7789_DrawImageRGB565() always accepts ordinary RGB565 values. */
#ifndef ST7789_IMAGE_BYTE_SWAPPED
#define ST7789_IMAGE_BYTE_SWAPPED 1
#endif
#ifndef ST7789_SPI_TIMEOUT_MS
#define ST7789_SPI_TIMEOUT_MS 1000U
#endif

/**
 *Color of pen
 *If you want to use another color, you can choose one in RGB565 format.
 */

#define WHITE       0xFFFF
#define BLACK       0x0000
#define BLUE        0x001F
#define RED         0xF800
#define MAGENTA     0xF81F
#define GREEN       0x07E0
#define CYAN        0x07FF
#define YELLOW      0xFFE0
#define GRAY        0X8430
#define BRED        0XF81F
#define GRED        0XFFE0
#define GBLUE       0X07FF
#define BROWN       0XBC40
#define BRRED       0XFC07
#define DARKBLUE    0X01CF
#define LIGHTBLUE   0X7D7C
#define GRAYBLUE    0X5458

#define LIGHTGREEN  0X9772
#define LGRAY       0XC618
#define LGRAYBLUE   0XA651
#define LBBLUE      0X2B12

/* Control Registers and constant codes */
#define ST7789_NOP     0x00
#define ST7789_SWRESET 0x01
#define ST7789_RDDID   0x04
#define ST7789_RDDST   0x09

#define ST7789_SLPIN   0x10
#define ST7789_SLPOUT  0x11
#define ST7789_PTLON   0x12
#define ST7789_NORON   0x13

#define ST7789_INVOFF  0x20
#define ST7789_INVON   0x21
#define ST7789_DISPOFF 0x28
#define ST7789_DISPON  0x29
#define ST7789_CASET   0x2A
#define ST7789_RASET   0x2B
#define ST7789_RAMWR   0x2C
#define ST7789_RAMRD   0x2E

#define ST7789_PTLAR   0x30
#define ST7789_COLMOD  0x3A
#define ST7789_MADCTL  0x36

/**
 * Memory Data Access Control Register (0x36H)
 * MAP:     D7  D6  D5  D4  D3  D2  D1  D0
 * param:   MY  MX  MV  ML  RGB MH  -   -
 *
 */

/* Page Address Order ('0': Top to Bottom, '1': the opposite) */
#define ST7789_MADCTL_MY  0x80
/* Column Address Order ('0': Left to Right, '1': the opposite) */
#define ST7789_MADCTL_MX  0x40
/* Page/Column Order ('0' = Normal Mode, '1' = Reverse Mode) */
#define ST7789_MADCTL_MV  0x20
/* Line Address Order ('0' = LCD Refresh Top to Bottom, '1' = the opposite) */
#define ST7789_MADCTL_ML  0x10
/* RGB/BGR Order ('0' = RGB, '1' = BGR) */
#define ST7789_MADCTL_RGB 0x00

#define ST7789_RDID1   0xDA
#define ST7789_RDID2   0xDB
#define ST7789_RDID3   0xDC
#define ST7789_RDID4   0xDD

/* Advanced options */
#define ST7789_COLOR_MODE_16bit 0x55    //  RGB565 (16bit)
#define ST7789_COLOR_MODE_18bit 0x66    //  RGB666 (18bit)

/* Basic operations */
#define ST7789_RST_Clr() HAL_GPIO_WritePin(ST7789_RST_PORT, ST7789_RST_PIN, GPIO_PIN_RESET)
#define ST7789_RST_Set() HAL_GPIO_WritePin(ST7789_RST_PORT, ST7789_RST_PIN, GPIO_PIN_SET)

#define ST7789_DC_Clr() HAL_GPIO_WritePin(ST7789_DC_PORT, ST7789_DC_PIN, GPIO_PIN_RESET)
#define ST7789_DC_Set() HAL_GPIO_WritePin(ST7789_DC_PORT, ST7789_DC_PIN, GPIO_PIN_SET)
#ifndef CFG_NO_CS
#define ST7789_Select() HAL_GPIO_WritePin(ST7789_CS_PORT, ST7789_CS_PIN, GPIO_PIN_RESET)
#define ST7789_UnSelect() HAL_GPIO_WritePin(ST7789_CS_PORT, ST7789_CS_PIN, GPIO_PIN_SET)
#else
#define ST7789_Select() ((void)0)
#define ST7789_UnSelect() ((void)0)
#endif

#define ABS(x) ((x) > 0 ? (x) : -(x))

/* Basic functions. */
void ST7789_Init(void);
void ST7789_SetRotation(uint8_t m);
void ST7789_Fill_Color(uint16_t color);
void ST7789_DrawPixel(uint16_t x, uint16_t y, uint16_t color);
void ST7789_Fill(uint16_t xSta, uint16_t ySta, uint16_t xEnd, uint16_t yEnd, uint16_t color);
void ST7789_DrawPixel_4px(uint16_t x, uint16_t y, uint16_t color);

/* Graphical functions. */
void ST7789_DrawLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);
void ST7789_DrawRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void ST7789_DrawCircle(uint16_t x0, uint16_t y0, uint8_t r, uint16_t color);
void ST7789_DrawImage(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *data);
void ST7789_DrawImageRGB565(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *data);
void ST7789_InvertColors(uint8_t invert);

/* Text functions. */
void ST7789_WriteChar(uint16_t x, uint16_t y, char ch, FontDef font, uint16_t color, uint16_t bgcolor);
void ST7789_WriteString(uint16_t x, uint16_t y, const char *str, FontDef font, uint16_t color, uint16_t bgcolor);

/* Extented Graphical functions. */
void ST7789_DrawFilledRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void ST7789_DrawTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint16_t color);
void ST7789_DrawFilledTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint16_t color);
void ST7789_DrawFilledCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);

void ST7789_DrawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color);
void ST7789_DrawFilledRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color);

void ST7789_DrawChar(uint16_t x, uint16_t y, char ch, FontDef font, uint16_t color, uint16_t bgcolor);
/* Command functions */
void ST7789_TearEffect(uint8_t tear);

/* Simple test function. */
void ST7789_Test(void);

#ifndef ST7789_ROTATION
    #error You should at least choose a display rotation!
#endif


/* Adafruit GFX font rendering. */
#ifdef __cplusplus
extern "C" {
#endif
/* Transparent foreground only. x/y is the glyph baseline origin.
 * Returns xAdvance, including for a space. Unsupported bytes become '?'
 * when the font contains it. No UTF-8 decoding, scaling or automatic wrap. */
uint8_t ST7789_WriteGFXChar(int16_t x, int16_t y, char ch,
                          const GFXfont *font, uint16_t color);
/* Newline advances yAdvance and resets x to the starting x; CR resets x. */
void ST7789_WriteGFXString(int16_t x, int16_t y, const char *text,
                         const GFXfont *font, uint16_t color);
#ifdef __cplusplus
}
#endif

/* Blocking preview: displays each enabled font for hold_ms, then returns.
 * Call after ST7789_Init(), from the main loop rather than an ISR. */
void ST7789_FontPreview(uint32_t hold_ms);

#endif /* __ST7789_H */
