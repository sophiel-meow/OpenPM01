#include "ui.h"

#ifndef NO_BATTERY
static int g_bat_pct = -1;
static int g_prev_bat_pct = -1;
static bool g_prev_bat_lo;

void ui_battery_update(void) { g_bat_pct = battery_pct(g_bus_v); }
#endif

static const tChar *g_arrow_up, *g_arrow_dn;

// format number
char *fmt_num(char *p, float val, int dec_places, int maxw) {
    char *start = p;
    float aval = val < 0 ? -val : val;
    int whole = (int)aval;
    int factor = (dec_places == 2) ? 100 : 1000;
    int dec = (int)((aval - (float)whole) * (float)factor + 0.5f);
    if (dec >= factor) {
        dec = 0;
        whole++;
    }
    int neg = (val < 0 && (whole > 0 || dec > 0));
    int nd = (whole >= 100) ? 3 : (whole >= 10) ? 2 : 1;
    int wid = neg + nd + 1 + dec_places;
    while (p - start < maxw - wid)
        *p++ = ' ';
    if (neg)
        *p++ = '-';
    if (whole >= 100)
        *p++ = '0' + (whole / 100) % 10;
    if (whole >= 10)
        *p++ = '0' + (whole / 10) % 10;
    *p++ = '0' + (whole % 10);
    *p++ = '.';
    if (dec_places == 2) {
        *p++ = '0' + (dec / 10) % 10;
        *p++ = '0' + (dec % 10);
    } else {
        *p++ = '0' + (dec / 100) % 10;
        *p++ = '0' + (dec / 10) % 10;
        *p++ = '0' + (dec % 10);
    }
    *p = '\0';
    return p;
}

// for temp & humid
char *fmt1d_str(char *p, const char *pfx, float val, const char *sfx) {
    while (*pfx)
        *p++ = *pfx++;
    float aval = val < 0 ? -val : val;
    int whole = (int)aval;
    int dec = (int)((aval - (float)whole) * 10.0f + 0.05f);
    if (dec > 9) {
        dec = 0;
        whole++;
    }
    if (val < 0 && (whole > 0 || dec > 0))
        *p++ = '-';
    if (whole >= 100)
        *p++ = '0' + (whole / 100) % 10;
    if (whole >= 10)
        *p++ = '0' + (whole / 10) % 10;
    *p++ = '0' + (whole % 10);
    *p++ = '.';
    *p++ = '0' + dec;
    // *p++ = ' ';
    while (*sfx)
        *p++ = *sfx++;
    *p = '\0';
    return p;
}

// drawing helpers
int str_width(const char *s, const tFont *f) {
    int w = 0;
    while (*s) {
        for (int i = 0; i < f->length; i++)
            if (f->chars[i].code == (long int)(unsigned char)*s) {
                w += f->chars[i].char_width;
                break;
            }
        s++;
    }
    return w;
}

void draw_val_unit(const char *num, uint16_t nfg, uint16_t nbg,
                   const tFont *nfont, const char *unit, uint16_t ufg,
                   uint16_t ubg, const tFont *ufont, uint16_t y) {
    int nw = str_width(num, nfont);
    int uw = str_width(unit, ufont);
    int total = nw + 4 + uw;
    int nx = (171 - 4) - total; /* RIGHT_X */
    int uy =
        y + (nfont->chars[0].char_height - ufont->chars[0].char_height) / 2;
    st_draw_string_withbg(nx, y, (char *)num, nfg, nbg, nfont);
    st_draw_string_withbg((171 - 4) - uw, uy, (char *)unit, ufg, ubg, ufont);
}

// draw character
void draw_ch(uint16_t x, uint16_t y, const tChar *ch, uint16_t fg,
             uint16_t bg) {
    if (ch)
        _st_render_glyph(x, y, fg, bg, ch->image, 1);
}

const tChar *font_find(const tFont *f, long int code) {
    for (int i = 0; i < f->length; i++)
        if (f->chars[i].code == code)
            return &f->chars[i];
    return NULL;
}

// shared alert helpers
#define ALERT_BLINK_MS 2000
#define ALERT_FONT (&CaskaydiaCoveNF_26)

static inline bool alert_show_msg(void) {
    return ((sys_ticks / ALERT_BLINK_MS) & 1);
}

static void draw_alert_msg(int x, int y, int w, int h, const char *msg,
                           uint16_t bg, const tFont *f) {
    int mw = str_width(msg, f);
    int my = y + (h - f->chars[0].char_height) / 2;
    st_fill_rect(x, y, w, h, bg);
    st_draw_string_withbg(x + (w - mw) / 2, my, (char *)msg, C_ALERT_FG, bg, f);
}

// UI init
void ui_init(void) {
    st_init();
    // orientation
#if defined(UI_LANDSCAPE)
    st_rotate_display(3);
    g_arrow_up = font_find(&CaskaydiaCoveNF_26, 0xe29782); // ◂ left  (neg curr)
    g_arrow_dn = font_find(&CaskaydiaCoveNF_26, 0xe296b8); // ▸ right (pos curr)
#elif defined(UI_LANDSCAPE_FLIP)
    st_rotate_display(2);
    g_arrow_up = font_find(&CaskaydiaCoveNF_26, 0xe296b8); // ▸ right (neg curr)
    g_arrow_dn = font_find(&CaskaydiaCoveNF_26, 0xe29782); // ◂ left  (pos curr)
#elif defined(UI_PORTRAIT_FLIP)
    st_rotate_display(1);
    g_arrow_up =
        font_find(&CaskaydiaCoveNF_26, 0xe296be); // ▼ (neg curr, flipped)
    g_arrow_dn =
        font_find(&CaskaydiaCoveNF_26, 0xe296b4); // ▲ (pos curr, flipped)
#else /* UI_PORTRAIT (default) */
    g_arrow_up = font_find(&CaskaydiaCoveNF_26, 0xe296b4); // ▲ (neg curr)
    g_arrow_dn = font_find(&CaskaydiaCoveNF_26, 0xe296be); // ▼ (pos curr)
#endif
    st_fill_screen(C_BG);
}

#if !defined(UI_LANDSCAPE) && !defined(UI_LANDSCAPE_FLIP)

#define STRIPE_X 0
#define STRIPE_W 4
#define MARGIN_L 6
#define RIGHT_X (171 - 4)

#define SEC_VOLT_Y 0
#define SEC_VOLT_H 74
#define SEC_CURR_Y (SEC_VOLT_Y + SEC_VOLT_H)
#define SEC_CURR_H 74
#define SEC_PWR_Y (SEC_CURR_Y + SEC_CURR_H)
#define SEC_PWR_H 64
#define SEC_CHARGE_Y (SEC_PWR_Y + SEC_PWR_H)
#define SEC_CHARGE_H 58
#define SEC_TH_Y (SEC_CHARGE_Y + SEC_CHARGE_H)
#define SEC_TH_H 48

static bool g_prev_va, g_prev_ia, g_prev_ta, g_prev_valid;
static bool g_prev_mah = true;
static int g_prev_blink = -1;

void ui_draw(void) {
    char nbuf[16];
    int blink = alert_show_msg();

    if (g_ina_ok) {
        // volt
        bool va = (g_bus_v < ALERT_VOLTAGE_LOW || g_bus_v > ALERT_VOLTAGE_HIGH);
        uint16_t vfg = va ? C_ALERT_FG : C_VOLT;
        uint16_t vbg = va ? C_ALERT : C_BG;
        uint16_t vstr = va ? C_ALERT : C_VOLT;
        st_fill_rect(STRIPE_X, SEC_VOLT_Y, STRIPE_W, SEC_VOLT_H, vstr);
        if (va != g_prev_va) {
            st_fill_rect(STRIPE_W, SEC_VOLT_Y, 172 - STRIPE_W, SEC_VOLT_H, vbg);
            g_prev_va = va;
        }
        st_draw_string_withbg(MARGIN_L, SEC_VOLT_Y + 2, "VOLT", C_LABEL, vbg,
                              &CaskaydiaCoveNF_18);

        int vy = SEC_VOLT_Y + SEC_VOLT_H - 43 - 2;
        if (va && blink) {
            if (blink != g_prev_blink)
                draw_alert_msg(STRIPE_W, vy, 172 - STRIPE_W, 43,
                               g_bus_v < ALERT_VOLTAGE_LOW ? "LOW VOLT."
                                                           : "HIGH VOLT.",
                               C_ALERT, ALERT_FONT);
        } else {
            if (va && blink != g_prev_blink)
                st_fill_rect(STRIPE_W, vy, 172 - STRIPE_W, 43, vbg);
            fmt2(nbuf, g_bus_v);
            draw_val_unit(nbuf, vfg, vbg, &CaskaydiaCoveNF_36, "V",
                          va ? vfg : C_UNIT, vbg, &CaskaydiaCoveNF_18, vy);
        }

        // curr
        bool ia = (g_current_a > ALERT_CURRENT_MAX ||
                   g_current_a < -ALERT_CURRENT_MAX);
        bool pos = (g_current_a >= 0.0f);
        uint16_t ifg = ia ? C_ALERT_FG : C_CURR_G;
        uint16_t ibg = ia ? C_ALERT : C_BG;
        uint16_t istr = ia ? C_ALERT : (pos ? C_CURR_G : C_CURR_R);
        st_fill_rect(STRIPE_X, SEC_CURR_Y, STRIPE_W, SEC_CURR_H, istr);
        if (ia != g_prev_ia) {
            st_fill_rect(STRIPE_W, SEC_CURR_Y, 172 - STRIPE_W, SEC_CURR_H, ibg);
            g_prev_ia = ia;
        }
        st_draw_string_withbg(MARGIN_L, SEC_CURR_Y + 2, "CURR", C_LABEL, ibg,
                              &CaskaydiaCoveNF_18);

        int lbl_w = str_width("CURR", &CaskaydiaCoveNF_18);
        int arrow_x = MARGIN_L + lbl_w + 6;
        int arrow_y = SEC_CURR_Y;

        if (g_current_valid != g_prev_valid) {
            st_fill_rect(arrow_x, arrow_y, 172 - arrow_x,
                         g_arrow_up->char_height, ibg);
            int val_y = SEC_CURR_Y + SEC_CURR_H - 43 - 2;
            st_fill_rect(STRIPE_W, val_y, 172 - STRIPE_W, 43, ibg);
            int pwry = SEC_PWR_Y + SEC_PWR_H - 43 - 2;
            st_fill_rect(STRIPE_W, pwry, 172 - STRIPE_W, 43, C_BG);
            g_prev_valid = g_current_valid;
        }

        int curr_y = SEC_CURR_Y + SEC_CURR_H - 43 - 2;
        if (ia && blink) {
            if (blink != g_prev_blink)
                draw_alert_msg(STRIPE_W, curr_y, 172 - STRIPE_W, 43,
                               "OVER CURR.", C_ALERT, ALERT_FONT);
        } else if (g_current_valid) {
            if (ia && blink != g_prev_blink)
                st_fill_rect(STRIPE_W, curr_y, 172 - STRIPE_W, 43, ibg);
            const tChar *arrow = pos ? g_arrow_dn : g_arrow_up;
            draw_ch(arrow_x, arrow_y, arrow, ifg, ibg);
            fmt3(nbuf, g_current_a < 0 ? -g_current_a : g_current_a);
            draw_val_unit(nbuf, ifg, ibg, &CaskaydiaCoveNF_36, "A",
                          ia ? ifg : C_UNIT, ibg, &CaskaydiaCoveNF_18, curr_y);
        } else {
            int dash_w = str_width("---", &CaskaydiaCoveNF_36);
            st_draw_string_withbg(RIGHT_X - dash_w, curr_y, "---", C_LABEL, ibg,
                                  &CaskaydiaCoveNF_36);
        }

        // pwr
        st_fill_rect(STRIPE_X, SEC_PWR_Y, STRIPE_W, SEC_PWR_H, C_PWR);
        st_draw_string_withbg(MARGIN_L, SEC_PWR_Y + 2, "PWR", C_LABEL, C_BG,
                              &CaskaydiaCoveNF_18);
        if (g_current_valid) {
            fmt2(nbuf, g_power_w);
            draw_val_unit(nbuf, C_PWR, C_BG, &CaskaydiaCoveNF_36, "W", C_UNIT,
                          C_BG, &CaskaydiaCoveNF_18,
                          SEC_PWR_Y + SEC_PWR_H - 43 - 2);
        } else {
            int dash_w = str_width("---", &CaskaydiaCoveNF_36);
            st_draw_string_withbg(RIGHT_X - dash_w,
                                  SEC_PWR_Y + SEC_PWR_H - 43 - 2, "---",
                                  C_LABEL, C_BG, &CaskaydiaCoveNF_36);
        }

        // charge
        st_fill_rect(STRIPE_X, SEC_CHARGE_Y, STRIPE_W, SEC_CHARGE_H, C_CHARGE);
        st_draw_string_withbg(MARGIN_L, SEC_CHARGE_Y + 2, "CHARGE", C_LABEL,
                              C_BG, &CaskaydiaCoveNF_18);
#ifndef NO_BATTERY
        {
            bool lo = (g_bus_v < BAT_LO_V);
            if (lo != g_prev_bat_lo) {
                int lw = str_width("LO BATT", &CaskaydiaCoveNF_18);
                int lh = CaskaydiaCoveNF_18.chars[0].char_height;
                st_fill_rect(RIGHT_X - lw, SEC_CHARGE_Y + 2, lw, lh, C_BG);
                if (lo)
                    st_draw_string_withbg(RIGHT_X - lw, SEC_CHARGE_Y + 2,
                                          "LO BATT", C_ALERT, C_BG,
                                          &CaskaydiaCoveNF_18);
                g_prev_bat_lo = lo;
            }
        }
#endif
        int chgy = SEC_CHARGE_Y + SEC_CHARGE_H - 32 + 1;
#ifndef NO_BATTERY
        chgy -= BAT_BAR_H + BAT_GAP; /* lift value to make room for bar */
#endif
        bool mah = (g_coulomb_mah < 1000.0f && g_coulomb_mah > -1000.0f);
        if (mah != g_prev_mah) {
            int clr_h = 32;
#ifndef NO_BATTERY
            clr_h += BAT_BAR_H + BAT_GAP;
#endif
            st_fill_rect(STRIPE_W, chgy, 172 - STRIPE_W, clr_h, C_BG);
            g_prev_mah = mah;
#ifndef NO_BATTERY
            g_prev_bat_pct = -1;
#endif
        }
        if (mah) {
            fmt2(nbuf, g_coulomb_mah);
            draw_val_unit(nbuf, C_UNIT, C_BG, &CaskaydiaCoveNF_26, "mAh",
                          C_UNIT, C_BG, &CaskaydiaCoveNF_18, chgy);
        } else {
            fmt2(nbuf, g_coulomb_mah / 1000.0f);
            draw_val_unit(nbuf, C_CHARGE, C_BG, &CaskaydiaCoveNF_26, "Ah",
                          C_CHARGE, C_BG, &CaskaydiaCoveNF_18, chgy);
        }
#ifndef NO_BATTERY
        {
            if (g_bat_pct != g_prev_bat_pct) {
                g_prev_bat_pct = g_bat_pct;
                int bar_y = chgy + 32 + BAT_GAP;
                int bar_w = RIGHT_X - MARGIN_L;
                st_fill_rect(MARGIN_L, bar_y, bar_w, BAT_BAR_H, CAT_SURFACE0);
                int fw = (bar_w * g_bat_pct + 50) / 100;
                if (fw > 0)
                    st_fill_rect(MARGIN_L, bar_y, fw, BAT_BAR_H, C_CHARGE);
            }
        }
#endif
    }

    // temp & humid
    if (g_sht_ok) {
        bool ta = (g_temp_c > ALERT_TEMP_HIGH || g_temp_c < ALERT_TEMP_LOW);
        uint16_t tfg = ta ? C_ALERT_FG : C_TEMP;
        uint16_t tbg = ta ? C_ALERT : C_BG;
        uint16_t tstr = ta ? C_ALERT : C_TEMP;
        st_fill_rect(STRIPE_X, SEC_TH_Y, STRIPE_W, SEC_TH_H, tstr);
        if (ta != g_prev_ta) {
            st_fill_rect(STRIPE_W, SEC_TH_Y, 172 - STRIPE_W, SEC_TH_H, tbg);
            g_prev_ta = ta;
        }
        if (ta && blink) {
            if (blink != g_prev_blink)
                draw_alert_msg(STRIPE_W, SEC_TH_Y, 172 - STRIPE_W, SEC_TH_H,
                               g_temp_c > ALERT_TEMP_HIGH ? "HIGH TEMP."
                                                          : "LOW TEMP.",
                               C_ALERT, ALERT_FONT);
        } else {
            if (ta && blink != g_prev_blink)
                st_fill_rect(STRIPE_W, SEC_TH_Y, 172 - STRIPE_W, SEC_TH_H, tbg);
            st_draw_string_withbg(MARGIN_L, SEC_TH_Y + 2, "TEMP", C_LABEL, tbg,
                                  &CaskaydiaCoveNF_18);
            st_draw_string_withbg(90, SEC_TH_Y + 2, "HUMID", C_LABEL, tbg,
                                  &CaskaydiaCoveNF_18);
            int thy = SEC_TH_Y + 24;
            const tChar *deg = font_find(&CaskaydiaCoveNF_18, 0xc2b0);
            fmt1d_str(nbuf, "", g_temp_c, "");
            int tx = MARGIN_L + 2;
            st_draw_string_withbg(tx, thy, nbuf, tfg, tbg, &CaskaydiaCoveNF_18);
            tx += str_width(nbuf, &CaskaydiaCoveNF_18);
            if (deg) {
                draw_ch(tx, thy, deg, tfg, tbg);
                tx += deg->char_width;
            }
            st_draw_string_withbg(tx, thy, "C", tfg, tbg, &CaskaydiaCoveNF_18);
            fmt1d_str(nbuf, "", g_hum_pct, "%");
            st_draw_string_withbg(90, thy, nbuf, C_TEMP, tbg,
                                  &CaskaydiaCoveNF_18);
        }
    }

    g_prev_blink = blink;
}

#else /* landscape (UI_LANDSCAPE or UI_LANDSCAPE_FLIP) — 320×172 */

// Two equal columns, three rows
#define LS_STRIPE_W 4
#define LS_MARGIN_L 6
#define LS_COL_W 160
#define LS_COL_L 0
#define LS_COL_R 160
#define LS_RIGHT_L (LS_COL_L + LS_COL_W - LS_STRIPE_W) /* 156 */
#define LS_RIGHT_R (LS_COL_R + LS_COL_W - LS_STRIPE_W) /* 316 */

// row layout
#define LS_ROW1_Y 0
#define LS_ROW1_H 80 /* VOLT | CURR */
#define LS_ROW2_Y 80
#define LS_ROW2_H 64 /* PWR  | CHARGE */
#define LS_ROW3_Y 144
#define LS_ROW3_H 28                                  /* TEMP | HUMID */

// Value y positions (absolute) — mirrors portrait font placement
#define LS_R1_VY (LS_ROW1_H - 43 - 2)                 /* 35  — 36pt */
#define LS_R2_PWR_VY (LS_ROW2_Y + LS_ROW2_H - 43 - 2) /* 99  — 36pt */
#define LS_R2_CHG_VY (LS_ROW2_Y + LS_ROW2_H - 32 + 1) /* 113 — 26pt */
#define LS_R3_THY (LS_ROW3_Y + (LS_ROW3_H - 22) / 2)  /* 147 — 18pt centred */

static bool g_prev_va, g_prev_ia, g_prev_ta, g_prev_valid;
static bool g_prev_mah = true;
static int g_prev_blink = -1;

// draw_val_unit with caller-supplied right edge
static void draw_val_unit_at(const char *num, uint16_t nfg, uint16_t nbg,
                             const tFont *nfont, const char *unit, uint16_t ufg,
                             uint16_t ubg, const tFont *ufont, int y,
                             int right_x) {
    int nw = str_width(num, nfont);
    int uw = str_width(unit, ufont);
    int nx = right_x - (nw + 4 + uw);
    int uy =
        y + (nfont->chars[0].char_height - ufont->chars[0].char_height) / 2;
    st_draw_string_withbg(nx, y, (char *)num, nfg, nbg, nfont);
    st_draw_string_withbg(right_x - uw, uy, (char *)unit, ufg, ubg, ufont);
}

void ui_draw(void) {
    char nbuf[16];
    int blink = alert_show_msg();

    if (g_ina_ok) {
        // volt
        bool va = (g_bus_v < ALERT_VOLTAGE_LOW || g_bus_v > ALERT_VOLTAGE_HIGH);
        uint16_t vfg = va ? C_ALERT_FG : C_VOLT;
        uint16_t vbg = va ? C_ALERT : C_BG;
        uint16_t vstr = va ? C_ALERT : C_VOLT;
        st_fill_rect(LS_COL_L, LS_ROW1_Y, LS_STRIPE_W, LS_ROW1_H, vstr);
        if (va != g_prev_va) {
            st_fill_rect(LS_COL_L + LS_STRIPE_W, LS_ROW1_Y,
                         LS_COL_W - LS_STRIPE_W, LS_ROW1_H, vbg);
            g_prev_va = va;
        }
        st_draw_string_withbg(LS_COL_L + LS_MARGIN_L, LS_ROW1_Y + 2, "VOLT",
                              C_LABEL, vbg, &CaskaydiaCoveNF_18);
        if (va && blink) {
            if (blink != g_prev_blink)
                draw_alert_msg(LS_COL_L + LS_STRIPE_W, LS_R1_VY,
                               LS_COL_W - LS_STRIPE_W, 43,
                               g_bus_v < ALERT_VOLTAGE_LOW ? "LOW VOLT."
                                                           : "HIGH VOLT.",
                               C_ALERT, ALERT_FONT);
        } else {
            if (va && blink != g_prev_blink)
                st_fill_rect(LS_COL_L + LS_STRIPE_W, LS_R1_VY,
                             LS_COL_W - LS_STRIPE_W, 43, vbg);
            fmt2(nbuf, g_bus_v);
            draw_val_unit_at(nbuf, vfg, vbg, &CaskaydiaCoveNF_36, "V",
                             va ? vfg : C_UNIT, vbg, &CaskaydiaCoveNF_18,
                             LS_R1_VY, LS_RIGHT_L);
        }

        // curr
        bool ia = (g_current_a > ALERT_CURRENT_MAX ||
                   g_current_a < -ALERT_CURRENT_MAX);
        bool pos = (g_current_a >= 0.0f);
        uint16_t ifg = ia ? C_ALERT_FG : C_CURR_G;
        uint16_t ibg = ia ? C_ALERT : C_BG;
        uint16_t istr = ia ? C_ALERT : (pos ? C_CURR_G : C_CURR_R);
        st_fill_rect(LS_COL_R, LS_ROW1_Y, LS_STRIPE_W, LS_ROW1_H, istr);
        if (ia != g_prev_ia) {
            st_fill_rect(LS_COL_R + LS_STRIPE_W, LS_ROW1_Y,
                         LS_COL_W - LS_STRIPE_W, LS_ROW1_H, ibg);
            g_prev_ia = ia;
        }
        st_draw_string_withbg(LS_COL_R + LS_MARGIN_L, LS_ROW1_Y + 2, "CURR",
                              C_LABEL, ibg, &CaskaydiaCoveNF_18);

        int lbl_w = str_width("CURR", &CaskaydiaCoveNF_18);
        int arrow_x = LS_COL_R + LS_MARGIN_L + lbl_w + 6;
        int arrow_y = LS_ROW1_Y;

        if (g_current_valid != g_prev_valid) {
            st_fill_rect(arrow_x, arrow_y, LS_COL_R + LS_COL_W - arrow_x,
                         g_arrow_up->char_height, ibg);
            st_fill_rect(LS_COL_R + LS_STRIPE_W, LS_R1_VY,
                         LS_COL_W - LS_STRIPE_W, 43, ibg);
            st_fill_rect(LS_COL_L + LS_STRIPE_W, LS_R2_PWR_VY,
                         LS_COL_W - LS_STRIPE_W, 43, C_BG);
            g_prev_valid = g_current_valid;
        }

        if (ia && blink) {
            if (blink != g_prev_blink)
                draw_alert_msg(LS_COL_R + LS_STRIPE_W, LS_R1_VY,
                               LS_COL_W - LS_STRIPE_W, 43, "OVER CURR.",
                               C_ALERT, ALERT_FONT);
        } else if (g_current_valid) {
            if (ia && blink != g_prev_blink)
                st_fill_rect(LS_COL_R + LS_STRIPE_W, LS_R1_VY,
                             LS_COL_W - LS_STRIPE_W, 43, ibg);
            const tChar *arrow = pos ? g_arrow_dn : g_arrow_up;
            draw_ch(arrow_x, arrow_y, arrow, ifg, ibg);
            /* maxw=0: no leading spaces — 36pt won't fit 7 chars in 160px col
             */
            fmt_num(nbuf, g_current_a < 0 ? -g_current_a : g_current_a, 3, 0);
            draw_val_unit_at(nbuf, ifg, ibg, &CaskaydiaCoveNF_36, "A",
                             ia ? ifg : C_UNIT, ibg, &CaskaydiaCoveNF_18,
                             LS_R1_VY, LS_RIGHT_R);
        } else {
            int dash_w = str_width("---", &CaskaydiaCoveNF_36);
            st_draw_string_withbg(LS_RIGHT_R - dash_w, LS_R1_VY, "---", C_LABEL,
                                  ibg, &CaskaydiaCoveNF_36);
        }

        // pwr
        st_fill_rect(LS_COL_L, LS_ROW2_Y, LS_STRIPE_W, LS_ROW2_H, C_PWR);
        st_draw_string_withbg(LS_COL_L + LS_MARGIN_L, LS_ROW2_Y + 2, "PWR",
                              C_LABEL, C_BG, &CaskaydiaCoveNF_18);
        if (g_current_valid) {
            fmt2(nbuf, g_power_w);
            draw_val_unit_at(nbuf, C_PWR, C_BG, &CaskaydiaCoveNF_36, "W",
                             C_UNIT, C_BG, &CaskaydiaCoveNF_18, LS_R2_PWR_VY,
                             LS_RIGHT_L);
        } else {
            int dash_w = str_width("---", &CaskaydiaCoveNF_36);
            st_draw_string_withbg(LS_RIGHT_L - dash_w, LS_R2_PWR_VY, "---",
                                  C_LABEL, C_BG, &CaskaydiaCoveNF_36);
        }

        // charge
        st_fill_rect(LS_COL_R, LS_ROW2_Y, LS_STRIPE_W, LS_ROW2_H, C_CHARGE);
        st_draw_string_withbg(LS_COL_R + LS_MARGIN_L, LS_ROW2_Y + 2, "CHARGE",
                              C_LABEL, C_BG, &CaskaydiaCoveNF_18);
#ifndef NO_BATTERY
        {
            bool lo = (g_bus_v < BAT_LO_V);
            if (lo != g_prev_bat_lo) {
                int lw = str_width("LO BATT", &CaskaydiaCoveNF_18);
                int lh = CaskaydiaCoveNF_18.chars[0].char_height;
                st_fill_rect(LS_RIGHT_R - lw, LS_ROW2_Y + 2, lw, lh, C_BG);
                if (lo)
                    st_draw_string_withbg(LS_RIGHT_R - lw, LS_ROW2_Y + 2,
                                          "LO BATT", C_ALERT, C_BG,
                                          &CaskaydiaCoveNF_18);
                g_prev_bat_lo = lo;
            }
        }
#endif
        int chg_vy = LS_R2_CHG_VY;
#ifndef NO_BATTERY
        chg_vy -= BAT_BAR_H + BAT_GAP;
#endif
        bool mah = (g_coulomb_mah < 1000.0f && g_coulomb_mah > -1000.0f);
        if (mah != g_prev_mah) {
            int clr_h = 32;
#ifndef NO_BATTERY
            clr_h += BAT_BAR_H + BAT_GAP;
#endif
            st_fill_rect(LS_COL_R + LS_STRIPE_W, chg_vy, LS_COL_W - LS_STRIPE_W,
                         clr_h, C_BG);
            g_prev_mah = mah;
#ifndef NO_BATTERY
            g_prev_bat_pct = -1;
#endif
        }
        if (mah) {
            fmt2(nbuf, g_coulomb_mah);
            draw_val_unit_at(nbuf, C_UNIT, C_BG, &CaskaydiaCoveNF_26, "mAh",
                             C_UNIT, C_BG, &CaskaydiaCoveNF_18, chg_vy,
                             LS_RIGHT_R);
        } else {
            fmt2(nbuf, g_coulomb_mah / 1000.0f);
            draw_val_unit_at(nbuf, C_CHARGE, C_BG, &CaskaydiaCoveNF_26, "Ah",
                             C_CHARGE, C_BG, &CaskaydiaCoveNF_18, chg_vy,
                             LS_RIGHT_R);
        }
#ifndef NO_BATTERY
        {
            if (g_bat_pct != g_prev_bat_pct) {
                g_prev_bat_pct = g_bat_pct;
                int bar_y = chg_vy + 32 + BAT_GAP;
                int bar_w = LS_RIGHT_R - (LS_COL_R + LS_MARGIN_L);
                st_fill_rect(LS_COL_R + LS_MARGIN_L, bar_y, bar_w, BAT_BAR_H,
                             CAT_SURFACE0);
                int fw = (bar_w * g_bat_pct + 50) / 100;
                if (fw > 0)
                    st_fill_rect(LS_COL_R + LS_MARGIN_L, bar_y, fw, BAT_BAR_H,
                                 C_CHARGE);
            }
        }
#endif
    }

    // temp & humid
    if (g_sht_ok) {
        bool ta = (g_temp_c > ALERT_TEMP_HIGH || g_temp_c < ALERT_TEMP_LOW);
        uint16_t tfg = ta ? C_ALERT_FG : C_TEMP;
        uint16_t tbg = ta ? C_ALERT : C_BG;
        uint16_t tstr = ta ? C_ALERT : C_TEMP;
        /* Stripes: TEMP column alerts; HUMID column is always teal */
        st_fill_rect(LS_COL_L, LS_ROW3_Y, LS_STRIPE_W, LS_ROW3_H, tstr);
        st_fill_rect(LS_COL_R, LS_ROW3_Y, LS_STRIPE_W, LS_ROW3_H, C_TEMP);
        if (ta != g_prev_ta) {
            st_fill_rect(LS_COL_L + LS_STRIPE_W, LS_ROW3_Y,
                         LS_COL_W - LS_STRIPE_W, LS_ROW3_H, tbg);
            g_prev_ta = ta;
        }

        int thy = LS_R3_THY;
        if (ta && blink) {
            if (blink != g_prev_blink)
                draw_alert_msg(LS_COL_L + LS_STRIPE_W, LS_ROW3_Y,
                               LS_COL_W - LS_STRIPE_W, LS_ROW3_H,
                               g_temp_c > ALERT_TEMP_HIGH ? "HIGH TEMP."
                                                          : "LOW TEMP.",
                               C_ALERT, &CaskaydiaCoveNF_18);
        } else {
            if (ta && blink != g_prev_blink)
                st_fill_rect(LS_COL_L + LS_STRIPE_W, LS_ROW3_Y,
                             LS_COL_W - LS_STRIPE_W, LS_ROW3_H, tbg);
            st_draw_string_withbg(LS_COL_L + LS_MARGIN_L, thy, "TEMP", C_LABEL,
                                  tbg, &CaskaydiaCoveNF_18);
            const tChar *deg = font_find(&CaskaydiaCoveNF_18, 0xc2b0);
            fmt1d_str(nbuf, "", g_temp_c, "");
            int tval_w = str_width(nbuf, &CaskaydiaCoveNF_18) +
                         (deg ? deg->char_width : 0) +
                         str_width("C", &CaskaydiaCoveNF_18);
            int tx = LS_RIGHT_L - tval_w;
            st_draw_string_withbg(tx, thy, nbuf, tfg, tbg, &CaskaydiaCoveNF_18);
            tx += str_width(nbuf, &CaskaydiaCoveNF_18);
            if (deg) {
                draw_ch(tx, thy, deg, tfg, tbg);
                tx += deg->char_width;
            }
            st_draw_string_withbg(tx, thy, "C", tfg, tbg, &CaskaydiaCoveNF_18);
        }

        // humid
        st_draw_string_withbg(LS_COL_R + LS_MARGIN_L, thy, "HUMID", C_LABEL,
                              C_BG, &CaskaydiaCoveNF_18);
        fmt1d_str(nbuf, "", g_hum_pct, "%");
        int hw = str_width(nbuf, &CaskaydiaCoveNF_18);
        st_draw_string_withbg(LS_RIGHT_R - hw, thy, nbuf, C_TEMP, C_BG,
                              &CaskaydiaCoveNF_18);
    }

    g_prev_blink = blink;
}

#endif
