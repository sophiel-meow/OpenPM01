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

// Adapted for 172x320 ST7789 with software SPI

#include "st7789_stm32_spi.h"

extern volatile uint32_t sys_ticks;

static void _st_delay_ms(uint32_t ms) {
    uint32_t end = sys_ticks + ms;
    while (sys_ticks < end)
        ;
}

// Display dimensions: 172x320
uint16_t st_tftwidth = 172;
uint16_t st_tftheight = 320;

/* Runtime offsets — swapped by st_rotate_display when MV is active.
   Portrait: x_off=34 (CASET=col dim), y_off=0  (RASET=row dim)
   Landscape (MV set): x_off=0  (CASET=row dim), y_off=34 (RASET=col dim) */
uint16_t st_x_off = ST_X_OFFSET;
uint16_t st_y_off = ST_Y_OFFSET;

void st_set_address_window(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
    uint16_t xs = x1 + st_x_off;
    uint16_t xe = x2 + st_x_off;
    uint16_t ys = y1 + st_y_off;
    uint16_t ye = y2 + st_y_off;

    _st_write_command_8bit(ST7789_CASET);

    ST_CS_ACTIVE;
    ST_DC_DAT;
    ST_WRITE_8BIT((uint8_t)(xs >> 8));
    ST_WRITE_8BIT((uint8_t)xs);
    ST_WRITE_8BIT((uint8_t)(xe >> 8));
    ST_WRITE_8BIT((uint8_t)xe);

    _st_write_command_8bit(ST7789_RASET);
    ST_CS_ACTIVE;
    ST_DC_DAT;
    ST_WRITE_8BIT((uint8_t)(ys >> 8));
    ST_WRITE_8BIT((uint8_t)ys);
    ST_WRITE_8BIT((uint8_t)(ye >> 8));
    ST_WRITE_8BIT((uint8_t)ye);

    _st_write_command_8bit(ST7789_RAMWR);
}

void _st_render_glyph(uint16_t x, uint16_t y, uint16_t fore_color,
                      uint16_t back_color, const tImage *glyph, uint8_t is_bg) {
    /* Column-burst: one address window per column, all pixels in one SPI burst.
       Eliminates the per-pixel CASET/RASET/RAMWR overhead (~14 bytes per pixel)
       and the visible scan-line tearing it causes. */
    (void)is_bg;

    int w = glyph->width;
    int h = glyph->height;
    const uint8_t *data = glyph->data;
    int col_bytes = (h + glyph->dataSize - 1) / glyph->dataSize;
    uint8_t fh = (uint8_t)(fore_color >> 8), fl = (uint8_t)fore_color;
    uint8_t bh = (uint8_t)(back_color >> 8), bl = (uint8_t)back_color;

    for (int col = 0; col < w; col++) {
        st_set_address_window(x + col, y, x + col, y + h - 1);
        ST_CS_ACTIVE;
        ST_DC_DAT;

        const uint8_t *cp = data + col * col_bytes;
        int bits_left = h;
        for (int b = 0; b < col_bytes && bits_left > 0; b++) {
            uint8_t d = cp[b];
            int n = bits_left < 8 ? bits_left : 8;
            for (int bit = 0; bit < n; bit++) {
                if (d & 0x80) {
                    ST_WRITE_8BIT(bh);
                    ST_WRITE_8BIT(bl);
                } else {
                    ST_WRITE_8BIT(fh);
                    ST_WRITE_8BIT(fl);
                }
                d <<= 1;
            }
            bits_left -= n;
        }
        ST_CS_IDLE;
    }
}

void _st_draw_string_main(uint16_t x, uint16_t y, char *str,
                          uint16_t fore_color, uint16_t back_color,
                          const tFont *font, uint8_t is_bg) {
    uint16_t x_temp = x;
    uint16_t y_temp = y;
    uint8_t x_padding = 0;
    uint8_t y_padding = 0;
    const tImage *img = NULL;
    uint16_t width = 0, height = 0;

    while (*str) {
        if (*str == '\n') {
            x_temp = x;
            y_temp += (font->chars[0].image->height + y_padding);
        } else if (*str == '\t') {
            x_temp += 4 * (font->chars[0].image->height + y_padding);
        } else {
            for (uint8_t i = 0; i < font->length; i++) {
                if (font->chars[i].code == *str) {
                    img = font->chars[i].image;
                    break;
                }
            }
            if (img == NULL)
                return;

            width = img->width;
            height = img->height;

            if (y_temp + (height + y_padding) > st_tftheight - 1)
                return;
            if (x_temp + (width + x_padding) > st_tftwidth - 1) {
                x_temp = x;
                y_temp += (height + y_padding);
            }

            if (is_bg)
                _st_render_glyph(x_temp, y_temp, fore_color, back_color, img,
                                 1);
            else
                _st_render_glyph(x_temp, y_temp, fore_color, back_color, img,
                                 0);
            x_temp += (width + x_padding);
        }

        str++;
    }
}

void st_draw_char(uint16_t x, uint16_t y, char character, uint16_t fore_color,
                  uint16_t back_color, const tFont *font, uint8_t is_bg) {
    const tImage *img = NULL;
    for (uint8_t i = 0; i < font->length; i++) {
        if (font->chars[i].code == character) {
            img = font->chars[i].image;
            break;
        }
    }
    if (img == NULL)
        return;

    if (is_bg)
        _st_render_glyph(x, y, fore_color, back_color, img, 1);
    else
        _st_render_glyph(x, y, fore_color, back_color, img, 0);
}

void st_draw_string(uint16_t x, uint16_t y, char *str, uint16_t color,
                    const tFont *font) {
    _st_draw_string_main(x, y, str, color, 0, font, 0);
}

void st_draw_string_withbg(uint16_t x, uint16_t y, char *str,
                           uint16_t fore_color, uint16_t back_color,
                           const tFont *font) {
    _st_draw_string_main(x, y, str, fore_color, back_color, font, 1);
}

void st_draw_bitmap(uint16_t x, uint16_t y, const tImage *bitmap) {
    uint16_t width = bitmap->width;
    uint16_t height = bitmap->height;
    st_set_address_window(x, y, x + width - 1, y + height - 1);

    ST_CS_ACTIVE;
    ST_DC_DAT;

    uint32_t total_pixels = (uint32_t)width * (uint32_t)height;
    for (uint32_t pixels = 0; pixels < total_pixels; pixels++) {
        ST_WRITE_8BIT((uint8_t)(bitmap->data[2 * pixels]));
        ST_WRITE_8BIT((uint8_t)(bitmap->data[2 * pixels + 1]));
    }

    ST_CS_IDLE;
}

void st_fill_color(uint16_t color, uint32_t len) {
    ST_CS_ACTIVE;
    ST_DC_DAT;

    uint32_t blocks = len / 8;
    uint32_t tail = len % 8;

    while (blocks--) {
        ST_WRITE_16BIT(color);
        ST_WRITE_16BIT(color);
        ST_WRITE_16BIT(color);
        ST_WRITE_16BIT(color);
        ST_WRITE_16BIT(color);
        ST_WRITE_16BIT(color);
        ST_WRITE_16BIT(color);
        ST_WRITE_16BIT(color);
    }
    while (tail--)
        ST_WRITE_16BIT(color);

    ST_CS_IDLE;
}

void st_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                  uint16_t color) {
    if (x >= st_tftwidth || y >= st_tftheight || w == 0 || h == 0)
        return;
    if (x + w - 1 >= st_tftwidth)
        w = st_tftwidth - x;
    if (y + h - 1 >= st_tftheight)
        h = st_tftheight - y;

    st_set_address_window(x, y, x + w - 1, y + h - 1);
    st_fill_color(color, (uint32_t)w * (uint32_t)h);
}

void st_fill_rect_fast(uint16_t x1, uint16_t y1, uint16_t w, uint16_t h,
                       uint16_t color) {
    st_set_address_window(x1, y1, x1 + w - 1, y1 + h - 1);
    st_fill_color(color, (uint32_t)w * (uint32_t)h);
}

void st_fill_screen(uint16_t color) {
    st_set_address_window(0, 0, st_tftwidth - 1, st_tftheight - 1);
    st_fill_color(color, (uint32_t)st_tftwidth * (uint32_t)st_tftheight);
}

void st_draw_rectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                       uint16_t color) {
    if (x >= st_tftwidth || y >= st_tftheight || w == 0 || h == 0)
        return;
    if (x + w - 1 >= st_tftwidth)
        w = st_tftwidth - x;
    if (y + h - 1 >= st_tftheight)
        h = st_tftheight - y;

    _st_draw_fast_h_line(x, y, x + w - 1, 1, color);
    _st_draw_fast_h_line(x, y + h, x + w - 1, 1, color);
    _st_draw_fast_v_line(x, y, y + h - 1, 1, color);
    _st_draw_fast_v_line(x + w, y, y + h - 1, 1, color);
}

void _st_plot_line_low(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1,
                       uint8_t width, uint16_t color) {
    int16_t dx = x1 - x0;
    int16_t dy = y1 - y0;
    int8_t yi = 1;
    uint8_t pixels_per_point = width * width;
    uint8_t color_high = (uint8_t)(color >> 8);
    uint8_t color_low = (uint8_t)color;

    if (dy < 0) {
        yi = -1;
        dy = -dy;
    }

    int16_t D = 2 * dy - dx;
    uint16_t y = y0;
    uint16_t x = x0;

    while (x <= x1) {
        st_set_address_window(x, y, x + width - 1, y + width - 1);
        ST_CS_ACTIVE;
        ST_DC_DAT;
        for (uint8_t pixel_cnt = 0; pixel_cnt < pixels_per_point; pixel_cnt++) {
            ST_WRITE_8BIT(color_high);
            ST_WRITE_8BIT(color_low);
        }
        ST_CS_IDLE;

        if (D > 0) {
            y = y + yi;
            D = D - 2 * dx;
        }
        D = D + 2 * dy;
        x++;
    }
}

void _st_plot_line_high(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1,
                        uint8_t width, uint16_t color) {
    int16_t dx = x1 - x0;
    int16_t dy = y1 - y0;
    int8_t xi = 1;
    uint8_t pixels_per_point = width * width;
    uint8_t color_high = (uint8_t)(color >> 8);
    uint8_t color_low = (uint8_t)color;

    if (dx < 0) {
        xi = -1;
        dx = -dx;
    }

    int16_t D = 2 * dx - dy;
    uint16_t y = y0;
    uint16_t x = x0;

    while (y <= y1) {
        st_set_address_window(x, y, x + width - 1, y + width - 1);
        ST_CS_ACTIVE;
        ST_DC_DAT;
        for (uint8_t pixel_cnt = 0; pixel_cnt < pixels_per_point; pixel_cnt++) {
            ST_WRITE_8BIT(color_high);
            ST_WRITE_8BIT(color_low);
        }
        ST_CS_IDLE;

        if (D > 0) {
            x = x + xi;
            D = D - 2 * dy;
        }
        D = D + 2 * dx;
        y++;
    }
}

void st_draw_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1,
                  uint8_t width, uint16_t color) {
    if (x0 == x1) {
        _st_draw_fast_v_line(x0, y0, y1, width, color);
    } else if (y0 == y1) {
        _st_draw_fast_h_line(x0, y0, x1, width, color);
    } else {
        if (abs(y1 - y0) < abs(x1 - x0)) {
            if (x0 > x1)
                _st_plot_line_low(x1, y1, x0, y0, width, color);
            else
                _st_plot_line_low(x0, y0, x1, y1, width, color);
        } else {
            if (y0 > y1)
                _st_plot_line_high(x1, y1, x0, y0, width, color);
            else
                _st_plot_line_high(x0, y0, x1, y1, width, color);
        }
    }
}

void _st_draw_fast_h_line(uint16_t x0, uint16_t y0, uint16_t x1, uint8_t width,
                          uint16_t color) {
    if (x0 < x1)
        st_set_address_window(x0, y0, x1, y0 + width - 1);
    else
        st_set_address_window(x1, y0, x0, y0 + width - 1);
    st_fill_color(color, (uint32_t)width * (uint32_t)abs(x1 - x0 + 1));
}

void _st_draw_fast_v_line(uint16_t x0, uint16_t y0, uint16_t y1, uint8_t width,
                          uint16_t color) {
    if (y0 < y1)
        st_set_address_window(x0, y0, x0 + width - 1, y1);
    else
        st_set_address_window(x0, y1, x0 + width - 1, y0);
    st_fill_color(color, (uint32_t)width * (uint32_t)abs(y1 - y0 + 1));
}

void st_draw_pixel(uint16_t x, uint16_t y, uint16_t color) {
    st_set_address_window(x, y, x, y);
    ST_CS_ACTIVE;
    ST_DC_DAT;
    ST_WRITE_8BIT((uint8_t)(color >> 8));
    ST_WRITE_8BIT((uint8_t)color);
    ST_CS_IDLE;
}

void st_rotate_display(uint8_t rotation) {
    rotation = rotation % 4;
    _st_write_command_8bit(ST7789_MADCTL);

    switch (rotation) {
    case 0:
        // Portrait: 172×320
        _st_write_data_8bit(ST7789_MADCTL_RGB);
        st_tftwidth  = 172; st_tftheight = 320;
        st_x_off = ST_X_OFFSET; st_y_off = ST_Y_OFFSET;
        break;
    case 1:
        // Portrait 180°: 172×320  (MX|MY, no MV — row/col not swapped)
        _st_write_data_8bit(ST7789_MADCTL_MX | ST7789_MADCTL_MY |
                            ST7789_MADCTL_RGB);
        st_tftwidth  = 172; st_tftheight = 320;
        st_x_off = ST_X_OFFSET; st_y_off = ST_Y_OFFSET;
        break;
    case 2:
        // Landscape (MV): 320×172  (MY|MV)
        _st_write_data_8bit(ST7789_MADCTL_MY | ST7789_MADCTL_MV |
                            ST7789_MADCTL_RGB);
        st_tftwidth  = 320; st_tftheight = 172;
        st_x_off = ST_Y_OFFSET; st_y_off = ST_X_OFFSET;  /* swap for MV */
        break;
    case 3:
        // Landscape (MV): 320×172  (MX|MV)
        _st_write_data_8bit(ST7789_MADCTL_MX | ST7789_MADCTL_MV |
                            ST7789_MADCTL_RGB);
        st_tftwidth  = 320; st_tftheight = 172;
        st_x_off = ST_Y_OFFSET; st_y_off = ST_X_OFFSET;  /* swap for MV */
        break;
    }
}

void st_init(void) {
    ST_CONFIG_GPIO_CLOCK();
    ST_CONFIG_GPIO();

    // Hardware reset
    ST_RST_ACTIVE;
    _st_delay_ms(1);
    ST_RST_IDLE;
    _st_delay_ms(5);

    // Software reset
    _st_write_command_8bit(ST7789_SWRESET);
    _st_delay_ms(5);

    // Sleep out
    _st_write_command_8bit(ST7789_SLPOUT);
    _st_delay_ms(5);

    // Color mode: 65K, 16-bit RGB565
    _st_write_command_8bit(ST7789_COLMOD);
    _st_write_data_8bit(ST7789_COLOR_MODE_65K | ST7789_COLOR_MODE_16BIT);
    _st_delay_ms(1);

    // MADCTL: Row/Column order
    _st_write_command_8bit(ST7789_MADCTL);
    _st_write_data_8bit(ST7789_MADCTL_RGB);

    // Porch control
    _st_write_command_8bit(ST7789_PORCTRL);
    _st_write_data_8bit(0x0C); // Back porch [2:0]=4, Front porch [2:0]=4
    _st_write_data_8bit(0x0C);
    _st_write_data_8bit(0x00);
    _st_write_data_8bit(0x33);
    _st_write_data_8bit(0x33);

    // Gate control
    _st_write_command_8bit(ST7789_GCTRL);
    _st_write_data_8bit(0x35); // VGH=13.26V, VGL=-10.43V

    // VCOM setting
    _st_write_command_8bit(ST7789_VCOMS);
    _st_write_data_8bit(0x28); // VCOM=0.9V

    // LCM control
    _st_write_command_8bit(ST7789_LCMCTRL);
    _st_write_data_8bit(0x2C);

    // VDV and VRH command enable
    _st_write_command_8bit(ST7789_VDVVRHEN);
    _st_write_data_8bit(0x01);
    _st_write_data_8bit(0xFF);

    // VRH set
    _st_write_command_8bit(ST7789_VRHS);
    _st_write_data_8bit(0x11);

    // VDV set
    _st_write_command_8bit(ST7789_VDVS);
    _st_write_data_8bit(0x20);

    // Frame rate control
    _st_write_command_8bit(ST7789_FRCTRL2);
    _st_write_data_8bit(0x0F); // 60Hz

    // Power control 1
    _st_write_command_8bit(ST7789_PWCTRL1);
    _st_write_data_8bit(0xA4);
    _st_write_data_8bit(0xA1);

    // Positive gamma
    _st_write_command_8bit(ST7789_PVGAMCTRL);
    _st_write_data_8bit(0xD0);
    _st_write_data_8bit(0x00);
    _st_write_data_8bit(0x05);
    _st_write_data_8bit(0x0E);
    _st_write_data_8bit(0x15);
    _st_write_data_8bit(0x0D);
    _st_write_data_8bit(0x37);
    _st_write_data_8bit(0x43);
    _st_write_data_8bit(0x47);
    _st_write_data_8bit(0x09);
    _st_write_data_8bit(0x15);
    _st_write_data_8bit(0x12);
    _st_write_data_8bit(0x16);
    _st_write_data_8bit(0x19);

    // Negative gamma
    _st_write_command_8bit(ST7789_NVGAMCTRL);
    _st_write_data_8bit(0xD0);
    _st_write_data_8bit(0x00);
    _st_write_data_8bit(0x05);
    _st_write_data_8bit(0x0D);
    _st_write_data_8bit(0x0C);
    _st_write_data_8bit(0x06);
    _st_write_data_8bit(0x2D);
    _st_write_data_8bit(0x44);
    _st_write_data_8bit(0x40);
    _st_write_data_8bit(0x0E);
    _st_write_data_8bit(0x1C);
    _st_write_data_8bit(0x18);
    _st_write_data_8bit(0x16);
    _st_write_data_8bit(0x19);

    // Inversion on
    _st_write_command_8bit(ST7789_INVON);
    _st_delay_ms(1);

    // Normal display on
    _st_write_command_8bit(ST7789_NORON);
    _st_delay_ms(1);

    // Display on
    _st_write_command_8bit(ST7789_DISPON);
    _st_delay_ms(1);

    // Turn on backlight
    ST_BLK_ON;
}
