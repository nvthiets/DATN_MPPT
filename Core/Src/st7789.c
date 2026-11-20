//nvthiets
#include "st7789.h"
#include "fonts.h"
#include <stddef.h>

#if defined(USING_135X240)
#define PANEL_W 135U
#define PANEL_H 240U
#elif defined(USING_240X240)
#define PANEL_W 240U
#define PANEL_H 240U
#elif defined(USING_170X320)
#define PANEL_W 170U
#define PANEL_H 320U
#else
#define PANEL_W 240U
#define PANEL_H 320U
#endif
#if (ST7789_ROTATION & 1)
uint16_t ST7789_Width = PANEL_H, ST7789_Height = PANEL_W;
#else
uint16_t ST7789_Width = PANEL_W, ST7789_Height = PANEL_H;
#endif
static uint16_t x_shift, y_shift;

/**
 * @brief Write command to ST7789 controller
 * @param cmd -> command to write
 * @return none
 */
static void ST7789_WriteCommand(uint8_t cmd)
{
	ST7789_Select();
	ST7789_DC_Clr();
	HAL_SPI_Transmit(&ST7789_SPI_PORT, &cmd, sizeof(cmd), ST7789_SPI_TIMEOUT_MS);
	ST7789_UnSelect();
}

/**
 * @brief Write data to ST7789 controller
 * @param buff -> pointer of data buffer
 * @param buff_size -> size of the data buffer
 * @return none
 */
static void ST7789_WriteData(const uint8_t *buff, size_t buff_size)
{
    ST7789_Select();
    ST7789_DC_Set();
    while (buff_size > 0U) {
        /* Even chunks also keep RGB565 pixel boundaries intact. */
        uint16_t chunk = buff_size > 65534U ? 65534U : (uint16_t)buff_size;
        HAL_StatusTypeDef status;
#ifdef USE_DMA
        if (chunk >= 16U) {
            status = HAL_SPI_Transmit_DMA(&ST7789_SPI_PORT, (uint8_t *)buff, chunk);
            if (status == HAL_OK) {
                uint32_t started = HAL_GetTick();
                while (HAL_SPI_GetState(&ST7789_SPI_PORT) != HAL_SPI_STATE_READY) {
                    if ((HAL_GetTick() - started) >= ST7789_SPI_TIMEOUT_MS) {
                        (void)HAL_SPI_Abort(&ST7789_SPI_PORT);
                        ST7789_UnSelect();
                        return;
                    }
                }
                if (HAL_SPI_GetError(&ST7789_SPI_PORT) != HAL_SPI_ERROR_NONE) {
                    ST7789_UnSelect();
                    return;
                }
            }
        } else
#endif
        {
            status = HAL_SPI_Transmit(&ST7789_SPI_PORT, (uint8_t *)buff,
                                      chunk, ST7789_SPI_TIMEOUT_MS);
        }
        if (status != HAL_OK) break;
        buff += chunk;
        buff_size -= chunk;
    }
    ST7789_UnSelect();
}
/**
 * @brief Write data to ST7789 controller, simplify for 8bit data.
 * data -> data to write
 * @return none
 */
static void ST7789_WriteSmallData(uint8_t data)
{
    ST7789_WriteData(&data, 1U);
}

/**
 * @brief Set the rotation direction of the display
 * @param m -> rotation parameter(please refer it in st7789.h)
 * @return none
 */
void ST7789_SetRotation(uint8_t m)
{
    static const uint8_t madctl[4] = {
        ST7789_MADCTL_MX | ST7789_MADCTL_MY,
        ST7789_MADCTL_MY | ST7789_MADCTL_MV,
        0,
        ST7789_MADCTL_MX | ST7789_MADCTL_MV
    };
    if (m > 3U) return;
    ST7789_Width = (m & 1U) ? PANEL_H : PANEL_W;
    ST7789_Height = (m & 1U) ? PANEL_W : PANEL_H;
#if defined(USING_135X240)
    static const uint16_t xs[4] = {53, 40, 52, 40};
    static const uint16_t ys[4] = {40, 52, 40, 53};
    x_shift = xs[m]; y_shift = ys[m];
#elif defined(USING_240X240)
    static const uint16_t xs[4] = {0, 80, 0, 0};
    static const uint16_t ys[4] = {80, 0, 0, 0};
    x_shift = xs[m]; y_shift = ys[m];
#elif defined(USING_170X320)
    x_shift = (m & 1U) ? 0U : 35U;
    y_shift = (m & 1U) ? 35U : 0U;
#else
    x_shift = 0; y_shift = 0;
#endif
    ST7789_WriteCommand(ST7789_MADCTL);
    ST7789_WriteSmallData(madctl[m] | ST7789_MADCTL_RGB);
}

/**
 * @brief Set address of DisplayWindow
 * @param xi&yi -> coordinates of window
 * @return none
 */
static void ST7789_SetAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{

	uint16_t x_start = x0 + x_shift, x_end = x1 + x_shift;
	uint16_t y_start = y0 + y_shift, y_end = y1 + y_shift;
	
	/* Column Address set */
	ST7789_WriteCommand(ST7789_CASET); 
	{
		uint8_t data[] = {x_start >> 8, x_start & 0xFF, x_end >> 8, x_end & 0xFF};
		ST7789_WriteData(data, sizeof(data));
	}

	/* Row Address set */
	ST7789_WriteCommand(ST7789_RASET);
	{
		uint8_t data[] = {y_start >> 8, y_start & 0xFF, y_end >> 8, y_end & 0xFF};
		ST7789_WriteData(data, sizeof(data));
	}
	/* Write to RAM */
	ST7789_WriteCommand(ST7789_RAMWR);

}

/**
 * @brief Initialize ST7789 controller
 * @param none
 * @return none
 */
void ST7789_Init(void)
{
	HAL_Delay(10);
    ST7789_RST_Clr();
    HAL_Delay(10);
    ST7789_RST_Set();
    HAL_Delay(20);

    ST7789_WriteCommand(ST7789_COLMOD);		//	Set color mode
    ST7789_WriteSmallData(ST7789_COLOR_MODE_16bit);
  	ST7789_WriteCommand(0xB2);				//	Porch control
	{
		uint8_t data[] = {0x0C, 0x0C, 0x00, 0x33, 0x33};
		ST7789_WriteData(data, sizeof(data));
	}
	ST7789_SetRotation(ST7789_ROTATION);	//	MADCTL (Display Rotation)
	
	/* Internal LCD Voltage generator settings */
    ST7789_WriteCommand(0XB7);				//	Gate Control
    ST7789_WriteSmallData(0x35);			//	Default value
    ST7789_WriteCommand(0xBB);				//	VCOM setting
    ST7789_WriteSmallData(0x19);			//	0.725v (default 0.75v for 0x20)
    ST7789_WriteCommand(0xC0);				//	LCMCTRL	
    ST7789_WriteSmallData (0x2C);			//	Default value
    ST7789_WriteCommand (0xC2);				//	VDV and VRH command Enable
    ST7789_WriteSmallData (0x01);			//	Default value
    ST7789_WriteCommand (0xC3);				//	VRH set
    ST7789_WriteSmallData (0x12);			//	+-4.45v (defalut +-4.1v for 0x0B)
    ST7789_WriteCommand (0xC4);				//	VDV set
    ST7789_WriteSmallData (0x20);			//	Default value
    ST7789_WriteCommand (0xC6);				//	Frame rate control in normal mode
    ST7789_WriteSmallData (0x0F);			//	Default value (60HZ)
    ST7789_WriteCommand (0xD0);				//	Power control
    ST7789_WriteSmallData (0xA4);			//	Default value
    ST7789_WriteSmallData (0xA1);			//	Default value
	/**************** Division line ****************/

	ST7789_WriteCommand(0xE0);
	{
		uint8_t data[] = {0xD0, 0x04, 0x0D, 0x11, 0x13, 0x2B, 0x3F, 0x54, 0x4C, 0x18, 0x0D, 0x0B, 0x1F, 0x23};
		ST7789_WriteData(data, sizeof(data));
	}

    ST7789_WriteCommand(0xE1);
	{
		uint8_t data[] = {0xD0, 0x04, 0x0C, 0x11, 0x13, 0x2C, 0x3F, 0x44, 0x51, 0x2F, 0x1F, 0x1F, 0x20, 0x23};
		ST7789_WriteData(data, sizeof(data));
	}

	  ST7789_WriteCommand (ST7789_INVOFF);	//	Inversion OFF	
	  ST7789_WriteCommand (ST7789_SLPOUT);
    HAL_Delay(120);	//	Out of sleep mode
  	ST7789_WriteCommand (ST7789_NORON);		//	Normal Display on
  	ST7789_WriteCommand (ST7789_DISPON);	//	Main screen turned on	

	HAL_Delay(50);
	ST7789_Fill_Color(BLACK);				//	Fill with Black.
}

/**
 * @brief Fill the DisplayWindow with single color
 * @param color -> color to Fill with
 * @return none
 */
void ST7789_Fill_Color(uint16_t color)
{
    ST7789_Fill(0, 0, ST7789_WIDTH - 1U, ST7789_HEIGHT - 1U, color);
}

/**
 * @brief Draw a Pixel
 * @param x&y -> coordinate to Draw
 * @param color -> color of the Pixel
 * @return none
 */
void ST7789_DrawPixel(uint16_t x, uint16_t y, uint16_t color)
{
    if (x >= ST7789_WIDTH || y >= ST7789_HEIGHT) return;
    ST7789_SetAddressWindow(x, y, x, y);
    uint8_t data[] = {(uint8_t)(color >> 8), (uint8_t)color};
    ST7789_WriteData(data, sizeof(data));
}

/**
 * @brief Fill an Area with single color
 * @param xSta&ySta -> coordinate of the start point
 * @param xEnd&yEnd -> coordinate of the end point
 * @param color -> color to Fill with
 * @return none
 */
void ST7789_Fill(uint16_t xSta, uint16_t ySta, uint16_t xEnd, uint16_t yEnd, uint16_t color)
{
    if (xSta >= ST7789_WIDTH || ySta >= ST7789_HEIGHT ||
        xEnd < xSta || yEnd < ySta) return;
    if (xEnd >= ST7789_WIDTH) xEnd = ST7789_WIDTH - 1U;
    if (yEnd >= ST7789_HEIGHT) yEnd = ST7789_HEIGHT - 1U;
    uint8_t buffer[256];
    for (size_t i = 0; i < sizeof(buffer); i += 2U) {
        buffer[i] = (uint8_t)(color >> 8);
        buffer[i + 1U] = (uint8_t)color;
    }
    ST7789_SetAddressWindow(xSta, ySta, xEnd, yEnd);
    uint32_t pixels = (uint32_t)(xEnd - xSta + 1U) * (yEnd - ySta + 1U);
    while (pixels > 0U) {
        uint16_t n = pixels > 128U ? 128U : (uint16_t)pixels;
        ST7789_WriteData(buffer, (size_t)n * 2U);
        pixels -= n;
    }
}

/**
 * @brief Draw a big Pixel at a point
 * @param x&y -> coordinate of the point
 * @param color -> color of the Pixel
 * @return none
 */
void ST7789_DrawPixel_4px(uint16_t x, uint16_t y, uint16_t color)
{
    /* Preserve the original centered 3x3 appearance; clip at the edges. */
    if (x >= ST7789_WIDTH || y >= ST7789_HEIGHT) return;
    ST7789_Fill(x ? x - 1U : 0U, y ? y - 1U : 0U, x + 1U, y + 1U, color);
}

/**
 * @brief Draw a line with single color
 * @param x1&y1 -> coordinate of the start point
 * @param x2&y2 -> coordinate of the end point
 * @param color -> color of the line to Draw
 * @return none
 */
void ST7789_DrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color)
{
    int32_t x = x0, y = y0;
    int32_t dx = ABS((int32_t)x1 - x0), dy = -ABS((int32_t)y1 - y0);
    int32_t sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1;
    int32_t err = dx + dy;
    /* DrawPixel rejects negative coordinates after uint16_t conversion. */
    for (;;) {
        ST7789_DrawPixel((uint16_t)x, (uint16_t)y, color);
        if (x == x1 && y == y1) break;
        int32_t e2 = 2 * err;
        if (e2 >= dy) { err += dy; x += sx; }
        if (e2 <= dx) { err += dx; y += sy; }
    }
}

/**
 * @brief Draw a Rectangle with single color
 * @param xi&yi -> 2 coordinates of 2 top points.
 * @param color -> color of the Rectangle line
 * @return none
 */
void ST7789_DrawRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{

	ST7789_DrawLine(x1, y1, x2, y1, color);
	ST7789_DrawLine(x1, y1, x1, y2, color);
	ST7789_DrawLine(x1, y2, x2, y2, color);
	ST7789_DrawLine(x2, y1, x2, y2, color);

}

/** 
 * @brief Draw a circle with single color
 * @param x0&y0 -> coordinate of circle center
 * @param r -> radius of circle
 * @param color -> color of circle line
 * @return  none
 */
void ST7789_DrawCircle(uint16_t x0, uint16_t y0, uint8_t r, uint16_t color)
{
	int16_t f = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x = 0;
	int16_t y = r;


	ST7789_DrawPixel(x0, y0 + r, color);
	ST7789_DrawPixel(x0, y0 - r, color);
	ST7789_DrawPixel(x0 + r, y0, color);
	ST7789_DrawPixel(x0 - r, y0, color);

	while (x < y) {
		if (f >= 0) {
			y--;
			ddF_y += 2;
			f += ddF_y;
		}
		x++;
		ddF_x += 2;
		f += ddF_x;

		ST7789_DrawPixel(x0 + x, y0 + y, color);
		ST7789_DrawPixel(x0 - x, y0 + y, color);
		ST7789_DrawPixel(x0 + x, y0 - y, color);
		ST7789_DrawPixel(x0 - x, y0 - y, color);

		ST7789_DrawPixel(x0 + y, y0 + x, color);
		ST7789_DrawPixel(x0 - y, y0 + x, color);
		ST7789_DrawPixel(x0 + y, y0 - x, color);
		ST7789_DrawPixel(x0 - y, y0 - x, color);
	}

}

/**
 * @brief Draw an Image on the screen
 * @param x&y -> start point of the Image
 * @param w&h -> width & height of the Image to Draw
 * @param data -> pointer of the Image array
 * @return none
 */
void ST7789_DrawImage(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *data)
{
#if ST7789_IMAGE_BYTE_SWAPPED
    if (!data || !w || !h || x >= ST7789_WIDTH || y >= ST7789_HEIGHT ||
        w > ST7789_WIDTH - x || h > ST7789_HEIGHT - y) return;
    ST7789_SetAddressWindow(x, y, x + w - 1U, y + h - 1U);
    ST7789_WriteData((const uint8_t *)data, (size_t)w * h * 2U);
#else
    ST7789_DrawImageRGB565(x, y, w, h, data);
#endif
}

void ST7789_DrawImageRGB565(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *data)
{
    if (!data || !w || !h || x >= ST7789_WIDTH || y >= ST7789_HEIGHT ||
        w > ST7789_WIDTH - x || h > ST7789_HEIGHT - y) return;
    uint8_t buffer[256];
    size_t left = (size_t)w * h;
    ST7789_SetAddressWindow(x, y, x + w - 1U, y + h - 1U);
    while (left) {
        size_t n = left > 128U ? 128U : left;
        for (size_t i = 0; i < n; ++i) {
            uint16_t pixel = *data++;
            buffer[2U * i] = (uint8_t)(pixel >> 8);
            buffer[2U * i + 1U] = (uint8_t)pixel;
        }
        ST7789_WriteData(buffer, n * 2U);
        left -= n;
    }
}

/**
 * @brief Invert Fullscreen color
 * @param invert -> Whether to invert
 * @return none
 */
void ST7789_InvertColors(uint8_t invert)
{

	ST7789_WriteCommand(invert ? 0x21 /* INVON */ : 0x20 /* INVOFF */);

}

/** 
 * @brief Write a char
 * @param  x&y -> cursor of the start point.
 * @param ch -> char to write
 * @param font -> fontstyle of the string
 * @param color -> color of the char
 * @param bgcolor -> background color of the char
 * @return  none
 */
void ST7789_WriteChar(uint16_t x, uint16_t y, char ch, FontDef font, uint16_t color, uint16_t bgcolor)
{
    uint8_t code = (uint8_t)ch;
    if (!font.data || !font.width || font.width > 16U || !font.height ||
        x >= ST7789_WIDTH || y >= ST7789_HEIGHT) return;
    if (code < 32U || code > 126U) code = '?';
    uint16_t w = font.width, h = font.height;
    if (w > ST7789_WIDTH - x) w = ST7789_WIDTH - x;
    if (h > ST7789_HEIGHT - y) h = ST7789_HEIGHT - y;
    uint8_t row[32];
    ST7789_SetAddressWindow(x, y, x + w - 1U, y + h - 1U);
    for (uint16_t i = 0; i < h; ++i) {
        uint16_t bits = font.data[(size_t)(code - 32U) * font.height + i];
        for (uint16_t j = 0; j < w; ++j) {
            uint16_t pixel = (bits & (0x8000U >> j)) ? color : bgcolor;
            row[2U * j] = (uint8_t)(pixel >> 8);
            row[2U * j + 1U] = (uint8_t)pixel;
        }
        ST7789_WriteData(row, (size_t)w * 2U);
    }
}

/** 
 * @brief Write a string 
 * @param  x&y -> cursor of the start point.
 * @param str -> string to write
 * @param font -> fontstyle of the string
 * @param color -> color of the string
 * @param bgcolor -> background color of the string
 * @return  none
 */
void ST7789_WriteString(uint16_t x, uint16_t y, const char *str, FontDef font, uint16_t color, uint16_t bgcolor)
{
    if (!str || !font.data || !font.width || font.width > 16U || !font.height ||
        x >= ST7789_WIDTH || y >= ST7789_HEIGHT) return;
    while (*str) {
        char ch = *str++;
        if (ch == '\r') { x = 0; continue; }
        if (ch == '\n') {
            if ((uint32_t)y + font.height >= ST7789_HEIGHT) break;
            x = 0; y += font.height; continue;
        }
        if ((uint32_t)x + font.width > ST7789_WIDTH) {
            x = 0;
            y += font.height;
            if (ch == ' ') continue;
        }
        if ((uint32_t)y + font.height > ST7789_HEIGHT) break;
        ST7789_WriteChar(x, y, ch, font, color, bgcolor);
        x += font.width;
    }
}

/** 
 * @brief Draw a filled Rectangle with single color
 * @param  x&y -> coordinates of the starting point
 * @param w&h -> width & height of the Rectangle
 * @param color -> color of the Rectangle
 * @return  none
 */
void ST7789_DrawFilledRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    if (w == 0U || h == 0U || x >= ST7789_WIDTH || y >= ST7789_HEIGHT) return;
    if (w > ST7789_WIDTH - x) w = ST7789_WIDTH - x;
    if (h > ST7789_HEIGHT - y) h = ST7789_HEIGHT - y;
    ST7789_Fill(x, y, x + w - 1U, y + h - 1U, color);
}

/** 
 * @brief Draw a Triangle with single color
 * @param  xi&yi -> 3 coordinates of 3 top points.
 * @param color ->color of the lines
 * @return  none
 */
void ST7789_DrawTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint16_t color)
{

	/* Draw lines */
	ST7789_DrawLine(x1, y1, x2, y2, color);
	ST7789_DrawLine(x2, y2, x3, y3, color);
	ST7789_DrawLine(x3, y3, x1, y1, color);

}

/** 
 * @brief Draw a filled Triangle with single color
 * @param  xi&yi -> 3 coordinates of 3 top points.
 * @param color ->color of the triangle
 * @return  none
 */
void ST7789_DrawFilledTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint16_t color)
{

	int16_t deltax = 0, deltay = 0, x = 0, y = 0, xinc1 = 0, xinc2 = 0,
			yinc1 = 0, yinc2 = 0, den = 0, num = 0, numadd = 0, numpixels = 0,
			curpixel = 0;

	deltax = ABS(x2 - x1);
	deltay = ABS(y2 - y1);
	x = x1;
	y = y1;

	if (x2 >= x1) {
		xinc1 = 1;
		xinc2 = 1;
	}
	else {
		xinc1 = -1;
		xinc2 = -1;
	}

	if (y2 >= y1) {
		yinc1 = 1;
		yinc2 = 1;
	}
	else {
		yinc1 = -1;
		yinc2 = -1;
	}

	if (deltax >= deltay) {
		xinc1 = 0;
		yinc2 = 0;
		den = deltax;
		num = deltax / 2;
		numadd = deltay;
		numpixels = deltax;
	}
	else {
		xinc2 = 0;
		yinc1 = 0;
		den = deltay;
		num = deltay / 2;
		numadd = deltax;
		numpixels = deltay;
	}

	for (curpixel = 0; curpixel <= numpixels; curpixel++) {
		ST7789_DrawLine(x, y, x3, y3, color);

		num += numadd;
		if (num >= den) {
			num -= den;
			x += xinc1;
			y += yinc1;
		}
		x += xinc2;
		y += yinc2;
	}

}

/** 
 * @brief Draw a Filled circle with single color
 * @param x0&y0 -> coordinate of circle center
 * @param r -> radius of circle
 * @param color -> color of circle
 * @return  none
 */
void ST7789_DrawFilledCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color)
{
    if (r < 0) return;

	int16_t f = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x = 0;
	int16_t y = r;

	ST7789_DrawPixel(x0, y0 + r, color);
	ST7789_DrawPixel(x0, y0 - r, color);
	ST7789_DrawPixel(x0 + r, y0, color);
	ST7789_DrawPixel(x0 - r, y0, color);
	ST7789_DrawLine(x0 - r, y0, x0 + r, y0, color);

	while (x < y) {
		if (f >= 0) {
			y--;
			ddF_y += 2;
			f += ddF_y;
		}
		x++;
		ddF_x += 2;
		f += ddF_x;

		ST7789_DrawLine(x0 - x, y0 + y, x0 + x, y0 + y, color);
		ST7789_DrawLine(x0 + x, y0 - y, x0 - x, y0 - y, color);

		ST7789_DrawLine(x0 + y, y0 + x, x0 - y, y0 + x, color);
		ST7789_DrawLine(x0 + y, y0 - x, x0 - y, y0 - x, color);
	}

}
void ST7789_DrawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color) {
    if (w <= 0 || h <= 0 || r < 0) return;
    if (r > (w - 1) / 2) r = (w - 1) / 2;
    if (r > (h - 1) / 2) r = (h - 1) / 2;

    ST7789_DrawLine(x + r, y, x + w - r - 1, y, color); 
    ST7789_DrawLine(x + r, y + h - 1, x + w - r - 1, y + h - 1, color); 
    ST7789_DrawLine(x, y + r, x, y + h - r - 1, color); 
    ST7789_DrawLine(x + w - 1, y + r, x + w - 1, y + h - r - 1, color); 

    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x1 = 0;
    int16_t y1 = r;

 
    
    while (x1 < y1) {
        if (f >= 0) {
            y1--;
            ddF_y += 2;
            f += ddF_y;
        }
        x1++;
        ddF_x += 2;
        f += ddF_x;

        ST7789_DrawPixel(x + w - r - 1 + x1, y + h - r - 1 + y1, color);
        ST7789_DrawPixel(x + w - r - 1 + y1, y + h - r - 1 + x1, color);
        ST7789_DrawPixel(x + r - x1, y + h - r - 1 + y1, color);
        ST7789_DrawPixel(x + r - y1, y + h - r - 1 + x1, color);
        ST7789_DrawPixel(x + w - r - 1 + x1, y + r - y1, color);
        ST7789_DrawPixel(x + w - r - 1 + y1, y + r - x1, color);
        ST7789_DrawPixel(x + r - x1, y + r - y1, color);
        ST7789_DrawPixel(x + r - y1, y + r - x1, color);
    }

}

static void ST7789_FillSignedRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
    int32_t right = (int32_t)x + w, bottom = (int32_t)y + h;
    if (w <= 0 || h <= 0 || right <= 0 || bottom <= 0) return;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x >= ST7789_WIDTH || y >= ST7789_HEIGHT) return;
    if (right > ST7789_WIDTH) right = ST7789_WIDTH;
    if (bottom > ST7789_HEIGHT) bottom = ST7789_HEIGHT;
    ST7789_DrawFilledRectangle(x, y, right - x, bottom - y, color);
}

void ST7789_DrawFilledRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color)
{
    if (w <= 0 || h <= 0 || r < 0) return;
    if (r > (w - 1) / 2) r = (w - 1) / 2;
    if (r > (h - 1) / 2) r = (h - 1) / 2;
    for (int32_t row = 0; row < h; ++row) {
        int32_t dy = 0;
        if (row < r) dy = r - row;
        else if (row >= h - r) dy = row - (h - r - 1);
        int32_t dx = r;
        while (dx * dx + dy * dy > (int32_t)r * r) --dx;
        int32_t inset = r - dx;
        int32_t yy = (int32_t)y + row;
        if (yy < 0 || yy >= ST7789_HEIGHT) continue;
        int32_t left = (int32_t)x + inset;
        int32_t right = (int32_t)x + w - inset;
        if (left < 0) left = 0;
        if (right > ST7789_WIDTH) right = ST7789_WIDTH;
        if (left < right) ST7789_FillSignedRect(left, yy, right - left, 1, color);
    }
}

void ST7789_DrawChar(uint16_t x, uint16_t y, char ch, FontDef font, uint16_t color, uint16_t bgcolor)
{
    ST7789_WriteChar(x, y, ch, font, color, bgcolor);
}
/**
 * @brief Open/Close tearing effect line
 * @param tear -> Whether to tear
 * @return none
 */
void ST7789_TearEffect(uint8_t tear)
{

	ST7789_WriteCommand(tear ? 0x35 /* TEON */ : 0x34 /* TEOFF */);
    if (tear) ST7789_WriteSmallData(0x00); /* V-blank TE mode. */

}


/** 
 * @brief A Simple test function for ST7789
 * @param  none
 * @return  none
 */
void ST7789_Test(void)
{
	ST7789_Fill_Color(WHITE);
	HAL_Delay(1000);
	ST7789_WriteString(10, 20, "Speed Test", Font_11x18, RED, WHITE);
	HAL_Delay(1000);
	ST7789_Fill_Color(CYAN);
    HAL_Delay(500);
	ST7789_Fill_Color(RED);
    HAL_Delay(500);
	ST7789_Fill_Color(BLUE);
    HAL_Delay(500);
	ST7789_Fill_Color(GREEN);
    HAL_Delay(500);
	ST7789_Fill_Color(YELLOW);
    HAL_Delay(500);
	ST7789_Fill_Color(BROWN);
    HAL_Delay(500);
	ST7789_Fill_Color(DARKBLUE);
    HAL_Delay(500);
	ST7789_Fill_Color(MAGENTA);
    HAL_Delay(500);
	ST7789_Fill_Color(LIGHTGREEN);
    HAL_Delay(500);
	ST7789_Fill_Color(LGRAY);
    HAL_Delay(500);
	ST7789_Fill_Color(LBBLUE);
    HAL_Delay(500);
	ST7789_Fill_Color(WHITE);
	HAL_Delay(500);

	ST7789_WriteString(10, 10, "Font test.", Font_16x26, GBLUE, WHITE);
	ST7789_WriteString(10, 50, "Hello Steve!", Font_7x10, RED, WHITE);
	ST7789_WriteString(10, 75, "Hello Steve!", Font_11x18, YELLOW, WHITE);
	ST7789_WriteString(10, 100, "Hello Steve!", Font_16x26, MAGENTA, WHITE);
	HAL_Delay(1000);

	ST7789_Fill_Color(RED);
	ST7789_WriteString(10, 10, "Rect./Line.", Font_11x18, YELLOW, BLACK);
	ST7789_DrawRectangle(30, 30, 100, 100, WHITE);
	HAL_Delay(1000);

	ST7789_Fill_Color(RED);
	ST7789_WriteString(10, 10, "Filled Rect.", Font_11x18, YELLOW, BLACK);
	ST7789_DrawFilledRectangle(30, 30, 50, 50, WHITE);
	HAL_Delay(1000);

	ST7789_Fill_Color(RED);
	ST7789_WriteString(10, 10, "Circle.", Font_11x18, YELLOW, BLACK);
	ST7789_DrawCircle(60, 60, 25, WHITE);
	HAL_Delay(1000);

	ST7789_Fill_Color(RED);
	ST7789_WriteString(10, 10, "Filled Cir.", Font_11x18, YELLOW, BLACK);
	ST7789_DrawFilledCircle(60, 60, 25, WHITE);
	HAL_Delay(1000);

	ST7789_Fill_Color(RED);
	ST7789_WriteString(10, 10, "Triangle", Font_11x18, YELLOW, BLACK);
	ST7789_DrawTriangle(30, 30, 30, 70, 60, 40, WHITE);
	HAL_Delay(1000);

	ST7789_Fill_Color(RED);
	ST7789_WriteString(10, 10, "Filled Tri", Font_11x18, YELLOW, BLACK);
	ST7789_DrawFilledTriangle(30, 30, 30, 70, 60, 40, WHITE);
	HAL_Delay(1000);

	//	If FLASH cannot storage anymore datas, please delete codes below.
	//ST7789_Fill_Color(WHITE);
	//ST7789_DrawImage(0, 0, 128, 128, (uint16_t *)saber);
	//HAL_Delay(3000);
}

/* Adafruit GFX font rendering. */
#include <stddef.h>
#include <limits.h>

static const GFXglyph *find_glyph(const GFXfont *font, uint8_t code)
{
    if (!font || !font->bitmap || !font->glyph || font->first > font->last)
        return NULL;
    if (code < font->first || code > font->last) {
        if ('?' < font->first || '?' > font->last) return NULL;
        code = '?';
    }
    return &font->glyph[code - font->first];
}

static void draw_glyph(int32_t x, int32_t y, const GFXfont *font,
                       const GFXglyph *g, uint16_t color)
{
    int32_t left = x + g->xOffset, top = y + g->yOffset;
    if (left >= ST7789_WIDTH || top >= ST7789_HEIGHT ||
        left + g->width <= 0 || top + g->height <= 0) return;
    /* Glyph bits are continuous across rows; no padding at row boundaries. */
    const uint8_t *bitmap = font->bitmap + g->bitmapOffset;
    for (uint16_t row = 0; row < g->height; ++row) {
        int32_t yy = top + row;
        if (yy < 0 || yy >= ST7789_HEIGHT) continue;
        uint16_t col = 0;
        while (col < g->width) {
            uint32_t bit = (uint32_t)row * g->width + col;
            if (!(bitmap[bit >> 3] & (0x80U >> (bit & 7U)))) {
                ++col;
                continue;
            }
            uint16_t start = col++;
            while (col < g->width) {
                bit = (uint32_t)row * g->width + col;
                if (!(bitmap[bit >> 3] & (0x80U >> (bit & 7U)))) break;
                ++col;
            }
            int32_t x0 = left + start, x1 = left + col - 1;
            if (x0 < 0) x0 = 0;
            if (x1 >= ST7789_WIDTH) x1 = ST7789_WIDTH - 1;
            if (x0 <= x1)
                ST7789_Fill((uint16_t)x0, (uint16_t)yy,
                            (uint16_t)x1, (uint16_t)yy, color);
        }
    }
}

uint8_t ST7789_WriteGFXChar(int16_t x, int16_t y, char ch,
                          const GFXfont *font, uint16_t color)
{
    const GFXglyph *g = find_glyph(font, (uint8_t)ch);
    if (!g) return 0;
    draw_glyph(x, y, font, g, color);
    return g->xAdvance;
}

void ST7789_WriteGFXString(int16_t x, int16_t y, const char *text,
                         const GFXfont *font, uint16_t color)
{
    if (!text || !font || !font->bitmap || !font->glyph) return;
    int32_t cursor_x = x, cursor_y = y;
    while (*text) {
        uint8_t code = (uint8_t)*text++;
        if (code == '\r') { cursor_x = x; continue; }
        if (code == '\n') {
            cursor_x = x;
            if (cursor_y > INT32_MAX - 255) return;
            cursor_y += font->yAdvance;
            continue;
        }
        const GFXglyph *g = find_glyph(font, code);
        if (!g) continue;
        /* Very long lines are skipped until CR/newline, without integer wrap. */
        if (cursor_x <= INT32_MAX - 512 && cursor_y <= INT32_MAX - 512)
            draw_glyph(cursor_x, cursor_y, font, g, color);
        if (cursor_x <= INT32_MAX - 255) cursor_x += g->xAdvance;
    }
}

/* Font preview registry, ordered by family, style, then size. */
typedef struct { const char *name; const GFXfont *font; } ST7789_PreviewEntry;
static const ST7789_PreviewEntry ST7789_PreviewFonts[] = {
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeMono9pt7b
    {"FreeMono9pt7b", &FreeMono9pt7b},
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeMono12pt7b
    {"FreeMono12pt7b", &FreeMono12pt7b},
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeMono18pt7b
    {"FreeMono18pt7b", &FreeMono18pt7b},
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeMono24pt7b
    {"FreeMono24pt7b", &FreeMono24pt7b},
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeMonoBold9pt7b
    {"FreeMonoBold9pt7b", &FreeMonoBold9pt7b},
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeMonoBold12pt7b
    {"FreeMonoBold12pt7b", &FreeMonoBold12pt7b},
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeMonoBold18pt7b
    {"FreeMonoBold18pt7b", &FreeMonoBold18pt7b},
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeMonoBold24pt7b
    {"FreeMonoBold24pt7b", &FreeMonoBold24pt7b},
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeMonoOblique9pt7b
    {"FreeMonoOblique9pt7b", &FreeMonoOblique9pt7b},
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeMonoOblique12pt7b
    {"FreeMonoOblique12pt7b", &FreeMonoOblique12pt7b},
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeMonoOblique18pt7b
    {"FreeMonoOblique18pt7b", &FreeMonoOblique18pt7b},
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeMonoOblique24pt7b
    {"FreeMonoOblique24pt7b", &FreeMonoOblique24pt7b},
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeMonoBoldOblique9pt7b
    {"FreeMonoBoldOblique9pt7b", &FreeMonoBoldOblique9pt7b},
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeMonoBoldOblique12pt7b
    {"FreeMonoBoldOblique12pt7b", &FreeMonoBoldOblique12pt7b},
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeMonoBoldOblique18pt7b
    {"FreeMonoBoldOblique18pt7b", &FreeMonoBoldOblique18pt7b},
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeMonoBoldOblique24pt7b
    {"FreeMonoBoldOblique24pt7b", &FreeMonoBoldOblique24pt7b},
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeSans9pt7b
    {"FreeSans9pt7b", &FreeSans9pt7b},
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeSans12pt7b
    {"FreeSans12pt7b", &FreeSans12pt7b},
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeSans18pt7b
    {"FreeSans18pt7b", &FreeSans18pt7b},
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeSans24pt7b
    {"FreeSans24pt7b", &FreeSans24pt7b},
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeSansBold9pt7b
    {"FreeSansBold9pt7b", &FreeSansBold9pt7b},
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeSansBold12pt7b
    {"FreeSansBold12pt7b", &FreeSansBold12pt7b},
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeSansBold18pt7b
    {"FreeSansBold18pt7b", &FreeSansBold18pt7b},
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeSansBold24pt7b
    {"FreeSansBold24pt7b", &FreeSansBold24pt7b},
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeSansOblique9pt7b
    {"FreeSansOblique9pt7b", &FreeSansOblique9pt7b},
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeSansOblique12pt7b
    {"FreeSansOblique12pt7b", &FreeSansOblique12pt7b},
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeSansOblique18pt7b
    {"FreeSansOblique18pt7b", &FreeSansOblique18pt7b},
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeSansOblique24pt7b
    {"FreeSansOblique24pt7b", &FreeSansOblique24pt7b},
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeSansBoldOblique9pt7b
    {"FreeSansBoldOblique9pt7b", &FreeSansBoldOblique9pt7b},
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeSansBoldOblique12pt7b
    {"FreeSansBoldOblique12pt7b", &FreeSansBoldOblique12pt7b},
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeSansBoldOblique18pt7b
    {"FreeSansBoldOblique18pt7b", &FreeSansBoldOblique18pt7b},
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeSansBoldOblique24pt7b
    {"FreeSansBoldOblique24pt7b", &FreeSansBoldOblique24pt7b},
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeSerif9pt7b
    {"FreeSerif9pt7b", &FreeSerif9pt7b},
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeSerif12pt7b
    {"FreeSerif12pt7b", &FreeSerif12pt7b},
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeSerif18pt7b
    {"FreeSerif18pt7b", &FreeSerif18pt7b},
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeSerif24pt7b
    {"FreeSerif24pt7b", &FreeSerif24pt7b},
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeSerifBold9pt7b
    {"FreeSerifBold9pt7b", &FreeSerifBold9pt7b},
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeSerifBold12pt7b
    {"FreeSerifBold12pt7b", &FreeSerifBold12pt7b},
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeSerifBold18pt7b
    {"FreeSerifBold18pt7b", &FreeSerifBold18pt7b},
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeSerifBold24pt7b
    {"FreeSerifBold24pt7b", &FreeSerifBold24pt7b},
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeSerifItalic9pt7b
    {"FreeSerifItalic9pt7b", &FreeSerifItalic9pt7b},
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeSerifItalic12pt7b
    {"FreeSerifItalic12pt7b", &FreeSerifItalic12pt7b},
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeSerifItalic18pt7b
    {"FreeSerifItalic18pt7b", &FreeSerifItalic18pt7b},
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeSerifItalic24pt7b
    {"FreeSerifItalic24pt7b", &FreeSerifItalic24pt7b},
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeSerifBoldItalic9pt7b
    {"FreeSerifBoldItalic9pt7b", &FreeSerifBoldItalic9pt7b},
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeSerifBoldItalic12pt7b
    {"FreeSerifBoldItalic12pt7b", &FreeSerifBoldItalic12pt7b},
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeSerifBoldItalic18pt7b
    {"FreeSerifBoldItalic18pt7b", &FreeSerifBoldItalic18pt7b},
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_FreeSerifBoldItalic24pt7b
    {"FreeSerifBoldItalic24pt7b", &FreeSerifBoldItalic24pt7b},
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_Org_01
    {"Org_01", &Org_01},
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_Picopixel
    {"Picopixel", &Picopixel},
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_Tiny3x3a2pt7b
    {"Tiny3x3a2pt7b", &Tiny3x3a2pt7b},
#endif
#if ST7789_ENABLE_ALL_GFX_FONTS || ST7789_USE_TomThumb
    {"TomThumb", &TomThumb},
#endif
    {0, 0} /* Sentinel keeps an empty font selection valid C. */
};
/*
void ST7789_FontPreview(uint32_t hold_ms)
{
    const uint16_t count = (uint16_t)(sizeof(ST7789_PreviewFonts) /
                                    sizeof(ST7789_PreviewFonts[0]) - 1U);
    if (count == 0U) {
        ST7789_Fill_Color(BLACK);
        ST7789_WriteString(10, 10, "No fonts enabled", Font_7x10, RED, BLACK);
        return;
    }
    for (uint16_t i = 0; i < count; ++i) {
        char page[] = "Font 00/00";
        page[5] = '0' + (i + 1U) / 10U;
        page[6] = '0' + (i + 1U) % 10U;
        page[8] = '0' + count / 10U;
        page[9] = '0' + count % 10U;
        ST7789_Fill_Color(BLACK);
        ST7789_WriteString(10, 8, page, Font_7x10, CYAN, BLACK);
        ST7789_WriteString(10, 25, ST7789_PreviewFonts[i].name, Font_7x10, WHITE, BLACK);
        ST7789_WriteGFXString(10, 85, "Aa Bb Cc", ST7789_PreviewFonts[i].font, WHITE);
        ST7789_WriteGFXString(10, 145, "012345", ST7789_PreviewFonts[i].font, YELLOW);
        ST7789_WriteGFXString(10, 205, "12.5V 2.3A", ST7789_PreviewFonts[i].font, GREEN);
        HAL_Delay(hold_ms);
    }
}*/
