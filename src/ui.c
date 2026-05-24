#include "ui.h"

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

// UI init
void ui_init(void) {
    st_init();
    st_fill_screen(C_BG);
    g_arrow_up = font_find(&CaskaydiaCoveNF_26, 0xe296b4);
    g_arrow_dn = font_find(&CaskaydiaCoveNF_26, 0xe296be);
}

// TODO: landscape UI
#ifndef UI_LANDSCAPE

#define STRIPE_X 0
#define STRIPE_W 4
#define MARGIN_L 6
#define RIGHT_X (171 - 4)

#define SEC_VOLT_Y 0
#define SEC_VOLT_H 78
#define SEC_CURR_Y (SEC_VOLT_Y + SEC_VOLT_H)
#define SEC_CURR_H 78
#define SEC_PWR_Y (SEC_CURR_Y + SEC_CURR_H)
#define SEC_PWR_H 64
#define SEC_CHARGE_Y (SEC_PWR_Y + SEC_PWR_H)
#define SEC_CHARGE_H 50
#define SEC_TH_Y (SEC_CHARGE_Y + SEC_CHARGE_H)
#define SEC_TH_H 48

static bool g_prev_va, g_prev_ia, g_prev_ta, g_prev_valid;

void ui_draw(void) {
    char nbuf[16];

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
        char *vlbl =
            va ? (g_bus_v < ALERT_VOLTAGE_LOW ? "VOLT LO" : "VOLT HI") : "VOLT";
        st_draw_string_withbg(MARGIN_L, SEC_VOLT_Y + 2, vlbl, C_LABEL, vbg,
                              &CaskaydiaCoveNF_18);
        fmt2(nbuf, g_bus_v);
        int vy = SEC_VOLT_Y + SEC_VOLT_H - 43 - 2;
        draw_val_unit(nbuf, vfg, vbg, &CaskaydiaCoveNF_36, "V", C_UNIT, vbg,
                      &CaskaydiaCoveNF_18, vy);

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
        char *clbl = ia ? "CURR OC" : "CURR";
        st_draw_string_withbg(MARGIN_L, SEC_CURR_Y + 2, clbl, C_LABEL, ibg,
                              &CaskaydiaCoveNF_18);

        int lbl_w = str_width(clbl, &CaskaydiaCoveNF_18);

        int arrow_x = MARGIN_L + lbl_w + 6;
        int arrow_y = SEC_CURR_Y;

        if (g_current_valid != g_prev_valid) {
            // arrow area
            st_fill_rect(arrow_x, arrow_y, 172 - arrow_x,
                         g_arrow_up->char_height, ibg);
            // current value row
            int val_y = SEC_CURR_Y + SEC_CURR_H - 43 - 2;
            st_fill_rect(STRIPE_W, val_y, 172 - STRIPE_W, 43, ibg);
            // power vaule row
            int pwry = SEC_PWR_Y + SEC_PWR_H - 43 - 2;
            st_fill_rect(STRIPE_W, pwry, 172 - STRIPE_W, 43, C_BG);
            g_prev_valid = g_current_valid;
        }

        if (g_current_valid) {
            const tChar *arrow = pos ? g_arrow_dn : g_arrow_up;
            draw_ch(arrow_x, arrow_y, arrow, ifg, ibg);
            int curr_y = SEC_CURR_Y + SEC_CURR_H - 43 - 2;
            fmt3(nbuf, g_current_a < 0 ? -g_current_a : g_current_a);
            draw_val_unit(nbuf, ifg, ibg, &CaskaydiaCoveNF_36, "A", C_UNIT, ibg,
                          &CaskaydiaCoveNF_18, curr_y);
        } else {
            int dash_w = str_width("---", &CaskaydiaCoveNF_36);
            st_draw_string_withbg(RIGHT_X - dash_w,
                                  SEC_CURR_Y + SEC_CURR_H - 43 - 2, "---",
                                  C_LABEL, ibg, &CaskaydiaCoveNF_36);
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
        int chgy = SEC_CHARGE_Y + SEC_CHARGE_H - 32 + 1;
        if (g_coulomb_mah < 1000.0f && g_coulomb_mah > -1000.0f) {
            fmt2(nbuf, g_coulomb_mah);
            draw_val_unit(nbuf, C_UNIT, C_BG, &CaskaydiaCoveNF_26, "mAh",
                          C_UNIT, C_BG, &CaskaydiaCoveNF_18, chgy);
        } else {
            fmt2(nbuf, g_coulomb_mah / 1000.0f);
            draw_val_unit(nbuf, C_CHARGE, C_BG, &CaskaydiaCoveNF_26, "Ah",
                          C_CHARGE, C_BG, &CaskaydiaCoveNF_18, chgy);
        }
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
        char *tlbl =
            ta ? (g_temp_c > ALERT_TEMP_HIGH ? "TEMP HI" : "TEMP LO") : "TEMP";
        st_draw_string_withbg(MARGIN_L, SEC_TH_Y + 2, tlbl, C_LABEL, tbg,
                              &CaskaydiaCoveNF_18);
        st_draw_string_withbg(90, SEC_TH_Y + 2, "HUMID", C_LABEL, C_BG,
                              &CaskaydiaCoveNF_18);
        int thy = SEC_TH_Y + 24;

        // temp number + degree C
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
        st_draw_string_withbg(90, thy, nbuf, C_TEMP, C_BG, &CaskaydiaCoveNF_18);
    }
}

#else

void ui_draw(void) { /* TODO: landscape layout */ }

#endif
