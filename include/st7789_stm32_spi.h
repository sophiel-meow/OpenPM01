/*
MIT License

Copyright (c) 2020 Avra Mitra

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

// Adapted for 172x320 ST7789 display with software SPI
// Pin mapping: RES=PA5, DC=PA4, SDA=PA6, SCL=PA7, CS=PA3

#include "fonts/bitmap_typedefs.h"
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <stdlib.h>

#ifndef INC_ST7789_STM32_SPI_H_
#define INC_ST7789_STM32_SPI_H_

#define ST7789_NOP 0x00
#define ST7789_SWRESET 0x01
#define ST7789_RDDID 0x04
#define ST7789_RDDST 0x09

#define ST7789_SLPIN 0x10
#define ST7789_SLPOUT 0x11
#define ST7789_PTLON 0x12
#define ST7789_NORON 0x13

#define ST7789_INVOFF 0x20
#define ST7789_INVON 0x21
#define ST7789_DISPOFF 0x28
#define ST7789_DISPON 0x29
#define ST7789_CASET 0x2A
#define ST7789_RASET 0x2B
#define ST7789_RAMWR 0x2C
#define ST7789_RAMRD 0x2E

#define ST7789_PTLAR 0x30
#define ST7789_COLMOD 0x3A
#define ST7789_MADCTL 0x36

#define ST7789_MADCTL_MY 0x80 // Page Address Order
#define ST7789_MADCTL_MX 0x40 // Column Address Order
#define ST7789_MADCTL_MV 0x20 // Page/Column Order
#define ST7789_MADCTL_ML 0x10 // Line Address Order
#define ST7789_MADCTL_MH 0x04 // Display Data Latch Order
#define ST7789_MADCTL_RGB 0x00
#define ST7789_MADCTL_BGR 0x08

#define ST7789_RDID1 0xDA
#define ST7789_RDID2 0xDB
#define ST7789_RDID3 0xDC
#define ST7789_RDID4 0xDD

// color modes
#define ST7789_COLOR_MODE_65K 0x50
#define ST7789_COLOR_MODE_262K 0x60
#define ST7789_COLOR_MODE_12BIT 0x03
#define ST7789_COLOR_MODE_16BIT 0x05
#define ST7789_COLOR_MODE_18BIT 0x06
#define ST7789_COLOR_MODE_16M 0x07

// Panel configuration registers
#define ST7789_PORCTRL 0xB2   // Porch control
#define ST7789_GCTRL 0xB7     // Gate control
#define ST7789_VCOMS 0xBB     // VCOM setting
#define ST7789_LCMCTRL 0xC0   // LCM control
#define ST7789_VDVVRHEN 0xC2  // VDV and VRH command enable
#define ST7789_VRHS 0xC3      // VRH set
#define ST7789_VDVS 0xC4      // VDV set
#define ST7789_FRCTRL2 0xC6   // Frame rate control
#define ST7789_PWCTRL1 0xD0   // Power control 1
#define ST7789_PVGAMCTRL 0xE0 // Positive voltage gamma
#define ST7789_NVGAMCTRL 0xE1 // Negative voltage gamma

// Display offset: 172x320 panel in 240x320 memory, centered
// X offset = (240 - 172) / 2 = 34
#define ST_X_OFFSET 34
#define ST_Y_OFFSET 0

// Color definitions (RGB565)

#define ST_R_POS_RGB 11
#define ST_G_POS_RGB 5
#define ST_B_POS_RGB 0

#define ST_RGB(R, G, B)                                                        \
    (((uint16_t)(R >> 3) << ST_R_POS_RGB) |                                    \
     ((uint16_t)(G >> 2) << ST_G_POS_RGB) |                                    \
     ((uint16_t)(B >> 3) << ST_B_POS_RGB))

#define ST_COLOR_BLACK ST_RGB(0, 0, 0)
#define ST_COLOR_NAVY ST_RGB(0, 0, 123)
#define ST_COLOR_DARKGREEN ST_RGB(0, 125, 0)
#define ST_COLOR_DARKCYAN ST_RGB(0, 125, 123)
#define ST_COLOR_MAROON ST_RGB(123, 0, 0)
#define ST_COLOR_PURPLE ST_RGB(123, 0, 123)
#define ST_COLOR_OLIVE ST_RGB(123, 125, 0)
#define ST_COLOR_LIGHTGREY ST_RGB(198, 195, 198)
#define ST_COLOR_DARKGREY ST_RGB(123, 125, 123)
#define ST_COLOR_BLUE ST_RGB(0, 0, 255)
#define ST_COLOR_GREEN ST_RGB(0, 255, 0)
#define ST_COLOR_CYAN ST_RGB(0, 255, 255)
#define ST_COLOR_RED ST_RGB(255, 0, 0)
#define ST_COLOR_MAGENTA ST_RGB(255, 0, 255)
#define ST_COLOR_YELLOW ST_RGB(255, 255, 0)
#define ST_COLOR_WHITE ST_RGB(255, 255, 255)
#define ST_COLOR_ORANGE ST_RGB(255, 165, 0)
#define ST_COLOR_GREENYELLOW ST_RGB(173, 255, 41)
#define ST_COLOR_PINK ST_RGB(255, 130, 198)

// Pin mapping:
// ST7789           STM32F103
// ---------------------------
// RES              PA5
// DC               PA4
// SDA (MOSI)       PA6
// SCL (SCK)        PA7
// CS               PA3
// BLK              PA2

#define ST_HAS_RST
#define ST_HAS_CS
#define ST_HAS_BLK

#define ST_PORT GPIOA

#define ST_RST GPIO5
#define ST_DC GPIO4
#define ST_SDA GPIO6
#define ST_SCL GPIO7
#define ST_CS GPIO3
#define ST_BLK GPIO2

// Pin control macros using BSRR/BRR for atomic set/reset
#define ST_DC_CMD GPIO_BRR(ST_PORT) = ST_DC
#define ST_DC_DAT GPIO_BSRR(ST_PORT) = ST_DC
#define ST_RST_ACTIVE GPIO_BRR(ST_PORT) = ST_RST
#define ST_RST_IDLE GPIO_BSRR(ST_PORT) = ST_RST
#define ST_CS_ACTIVE GPIO_BRR(ST_PORT) = ST_CS
#define ST_CS_IDLE GPIO_BSRR(ST_PORT) = ST_CS

#define ST_SDA_HIGH GPIO_BSRR(ST_PORT) = ST_SDA
#define ST_SDA_LOW GPIO_BRR(ST_PORT) = ST_SDA
#define ST_SCL_HIGH GPIO_BSRR(ST_PORT) = ST_SCL
#define ST_SCL_LOW GPIO_BRR(ST_PORT) = ST_SCL
// Backlight active-low: ON = pin LOW, OFF = pin HIGH
#define ST_BLK_ON GPIO_BRR(ST_PORT) = ST_BLK
#define ST_BLK_OFF GPIO_BSRR(ST_PORT) = ST_BLK

// Software SPI bit-bang: CPOL=1 (idle high), CPHA=0 (sample on first edge)
// MSB first — minimal nops for max throughput (~12 Mbps)
#define ST_WRITE_BIT(_b)                                                       \
    do {                                                                       \
        if (_b) {                                                              \
            ST_SDA_HIGH;                                                       \
        } else {                                                               \
            ST_SDA_LOW;                                                        \
        }                                                                      \
        ST_SCL_LOW;                                                            \
        __asm__("nop");                                                        \
        ST_SCL_HIGH;                                                           \
    } while (0)

#define ST_WRITE_8BIT(d)                                                       \
    do {                                                                       \
        uint8_t _d = (d);                                                      \
        ST_WRITE_BIT(_d & 0x80);                                               \
        ST_WRITE_BIT(_d & 0x40);                                               \
        ST_WRITE_BIT(_d & 0x20);                                               \
        ST_WRITE_BIT(_d & 0x10);                                               \
        ST_WRITE_BIT(_d & 0x08);                                               \
        ST_WRITE_BIT(_d & 0x04);                                               \
        ST_WRITE_BIT(_d & 0x02);                                               \
        ST_WRITE_BIT(_d & 0x01);                                               \
    } while (0)

/* Write a 16-bit RGB565 pixel in one unrolled burst */
#define ST_WRITE_16BIT(d)                                                      \
    do {                                                                       \
        uint16_t _d = (d);                                                     \
        ST_WRITE_BIT(_d & 0x8000);                                             \
        ST_WRITE_BIT(_d & 0x4000);                                             \
        ST_WRITE_BIT(_d & 0x2000);                                             \
        ST_WRITE_BIT(_d & 0x1000);                                             \
        ST_WRITE_BIT(_d & 0x0800);                                             \
        ST_WRITE_BIT(_d & 0x0400);                                             \
        ST_WRITE_BIT(_d & 0x0200);                                             \
        ST_WRITE_BIT(_d & 0x0100);                                             \
        ST_WRITE_BIT(_d & 0x0080);                                             \
        ST_WRITE_BIT(_d & 0x0040);                                             \
        ST_WRITE_BIT(_d & 0x0020);                                             \
        ST_WRITE_BIT(_d & 0x0010);                                             \
        ST_WRITE_BIT(_d & 0x0008);                                             \
        ST_WRITE_BIT(_d & 0x0004);                                             \
        ST_WRITE_BIT(_d & 0x0002);                                             \
        ST_WRITE_BIT(_d & 0x0001);                                             \
    } while (0)

// GPIO and clock configuration
#define ST_CONFIG_GPIO_CLOCK()                                                 \
    {                                                                          \
        rcc_periph_clock_enable(RCC_GPIOA);                                    \
        rcc_periph_clock_enable(RCC_AFIO);                                     \
    }

#define ST_CONFIG_GPIO()                                                       \
    {                                                                          \
        /* All pins as push-pull outputs: SDA, SCL, DC, RST, CS, BLK */        \
        gpio_set_mode(ST_PORT, GPIO_MODE_OUTPUT_50_MHZ,                        \
                      GPIO_CNF_OUTPUT_PUSHPULL,                                \
                      ST_SDA | ST_SCL | ST_DC | ST_RST | ST_CS | ST_BLK);      \
        /* Set initial levels: CS idle high, RST idle high, DC high, SCL idle  \
         * high, BLK off (active-low, high=off) */                             \
        gpio_set(ST_PORT, ST_CS | ST_RST | ST_DC | ST_SCL | ST_BLK);           \
        /* Release JTAG on PB4 so it can be used as GPIO if needed */          \
        AFIO_MAPR |= AFIO_MAPR_SWJ_CFG_FULL_SWJ_NO_JNTRST;                     \
    }

#define ST_SWAP(a, b)                                                          \
    {                                                                          \
        uint16_t temp;                                                         \
        temp = a;                                                              \
        a = b;                                                                 \
        b = temp;                                                              \
    }

extern uint16_t st_tftwidth;
extern uint16_t st_tftheight;
extern uint16_t st_x_off;
extern uint16_t st_y_off;

/*
 * inline function to send 8 bit command to the display
 */
__attribute__((always_inline)) static inline void
_st_write_command_8bit(uint8_t cmd) {
    ST_CS_ACTIVE;
    ST_DC_CMD;
    ST_WRITE_8BIT(cmd);
    ST_CS_IDLE;
}

/*
 * inline function to send 8 bit data to the display
 */
__attribute__((always_inline)) static inline void
_st_write_data_8bit(uint8_t dat) {
    ST_CS_ACTIVE;
    ST_DC_DAT;
    ST_WRITE_8BIT(dat);
    ST_CS_IDLE;
}

/*
 * inline function to send 16 bit data to the display
 */
__attribute__((always_inline)) static inline void
_st_write_data_16bit(uint16_t dat) {
    ST_CS_ACTIVE;
    ST_DC_DAT;
    ST_WRITE_8BIT((uint8_t)(dat >> 8));
    ST_WRITE_8BIT((uint8_t)dat);
    ST_CS_IDLE;
}

/*
 * Write multiple bytes over software SPI with CS active throughout
 */
static inline void _st_write_data_burst(const uint8_t *data, uint32_t len) {
    ST_CS_ACTIVE;
    ST_DC_DAT;
    while (len--) {
        uint8_t d = *data++;
        for (uint8_t i = 0; i < 8; i++) {
            if (d & 0x80) {
                ST_SDA_HIGH;
            } else {
                ST_SDA_LOW;
            }
            ST_SCL_LOW;
            __asm__("nop");
            ST_SCL_HIGH;
            d <<= 1;
        }
    }
    ST_CS_IDLE;
}

void st_set_address_window(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void st_fill_color(uint16_t color, uint32_t len);
void st_draw_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1,
                  uint8_t width, uint16_t color);
void st_draw_rectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                       uint16_t color);
void _st_plot_line_low(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1,
                       uint8_t width, uint16_t color);
void _st_plot_line_high(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1,
                        uint8_t width, uint16_t color);
void _st_draw_fast_h_line(uint16_t x0, uint16_t y0, uint16_t x1, uint8_t width,
                          uint16_t color);
void _st_draw_fast_v_line(uint16_t x0, uint16_t y0, uint16_t y1, uint8_t width,
                          uint16_t color);
void st_rotate_display(uint8_t rotation);
void st_init(void);
void st_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                  uint16_t color);
void st_fill_rect_fast(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                       uint16_t color);
void st_fill_screen(uint16_t color);
void _st_render_glyph(uint16_t x, uint16_t y, uint16_t fore_color,
                      uint16_t back_color, const tImage *glyph, uint8_t is_bg);
void _st_draw_string_main(uint16_t x, uint16_t y, char *str,
                          uint16_t fore_color, uint16_t back_color,
                          const tFont *font, uint8_t is_bg);
void st_draw_char(uint16_t x, uint16_t y, char character, uint16_t fore_color,
                  uint16_t back_color, const tFont *font, uint8_t is_bg);
void st_draw_string(uint16_t x, uint16_t y, char *str, uint16_t color,
                    const tFont *font);
void st_draw_string_withbg(uint16_t x, uint16_t y, char *str,
                           uint16_t fore_color, uint16_t back_color,
                           const tFont *font);
void st_draw_bitmap(uint16_t x, uint16_t y, const tImage *bitmap);
void st_draw_pixel(uint16_t x, uint16_t y, uint16_t color);

#endif /* INC_ST7789_STM32_SPI_H_ */
