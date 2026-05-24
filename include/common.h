#ifndef COMMON_H
#define COMMON_H

#include "colour.h"
#include "fonts/CaskaydiaCoveNF_18.h"
#include "fonts/CaskaydiaCoveNF_26.h"
#include "fonts/CaskaydiaCoveNF_36.h"
#include "sensors.h"
#include "st7789_stm32_spi.h"
#include <stdbool.h>
#include <stdint.h>

// task periods (ms)
#define INA226_PERIOD_MS 35
#define SHT30_PERIOD_MS 2000
#define UI_PERIOD_MS 50

// alert thresholds
#define ALERT_VOLTAGE_LOW 5.0f
#define ALERT_VOLTAGE_HIGH 35.0f
#define ALERT_CURRENT_MAX 30.0f
#define ALERT_TEMP_HIGH 50.0f
#define ALERT_TEMP_LOW -40.0f

// sensor state
#define g_temp_c sensors_temp()
#define g_hum_pct sensors_humidity()
#define g_bus_v sensors_voltage()
#define g_current_a sensors_current()
#define g_power_w sensors_power()
#define g_coulomb_mah sensors_coulomb_mah()
#define g_sht_ok sensors_sht_ok()
#define g_ina_ok sensors_ina_ok()
#define g_current_valid sensors_current_valid()

// tick
extern volatile uint32_t sys_ticks;

// drawing
int str_width(const char *s, const tFont *f);
void draw_val_unit(const char *num, uint16_t nfg, uint16_t nbg,
                   const tFont *nfont, const char *unit, uint16_t ufg,
                   uint16_t ubg, const tFont *ufont, uint16_t y);
void draw_ch(uint16_t x, uint16_t y, const tChar *ch, uint16_t fg, uint16_t bg);
const tChar *font_find(const tFont *f, long int code);

// formatting
char *fmt_num(char *p, float val, int dec_places, int maxw);
char *fmt1d_str(char *p, const char *pfx, float val, const char *sfx);

#define fmt3(buf, val) fmt_num(buf, val, 3, 7)
#define fmt2(buf, val) fmt_num(buf, val, 2, 6)
/* #define fmt3c(buf, val) fmt_num(buf, val, 3, 6) */

#endif
