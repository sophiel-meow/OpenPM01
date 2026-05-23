#include "ina226.h"
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/i2c.h>
#include <libopencm3/stm32/rcc.h>

#define REG_CONFIG 0x00
#define REG_SHUNT_V 0x01
#define REG_BUS_V 0x02
#define REG_POWER 0x03
#define REG_CURRENT 0x04
#define REG_CALIB 0x05
#define REG_MFR_ID 0xFE
#define REG_DIE_ID 0xFF

static float _current_lsb = 0.0005f;
static float _current_offset = 0.0f;

// hardware i2c helpers
static uint16_t _read_reg(uint8_t addr, uint8_t reg) {
    uint8_t rb = reg;
    uint8_t data[2] = {0, 0};
    i2c_transfer7(I2C1, addr, &rb, 1, data, 2);
    return ((uint16_t)data[0] << 8) | data[1];
}

static void _write_reg(uint8_t addr, uint8_t reg, uint16_t val) {
    uint8_t buf[3] = {reg, (uint8_t)(val >> 8), (uint8_t)(val & 0xFF)};
    i2c_transfer7(I2C1, addr, buf, 3, NULL, 0);
}

/* public */
void ina226_init(void) {
    rcc_periph_clock_enable(RCC_I2C1);
    rcc_periph_clock_enable(RCC_GPIOB);
    rcc_periph_clock_enable(RCC_AFIO);

    gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ,
                  GPIO_CNF_OUTPUT_ALTFN_OPENDRAIN, GPIO6 | GPIO7);

    i2c_peripheral_disable(I2C1);
    i2c_set_speed(I2C1, i2c_speed_sm_100k, rcc_apb1_frequency / 1000000);
    i2c_peripheral_enable(I2C1);
}

bool ina226_is_connected(uint8_t addr) {
    return (ina226_read_mfr_id(addr) == 0x5449);
}

int ina226_configure(uint8_t addr, float shunt_ohm, float max_current_a) {
    float sv = max_current_a * shunt_ohm;

    if (sv > 0.08192f)
        return -1;
    if (max_current_a < 0.001f)
        return -2;
    if (shunt_ohm < 0.0001f)
        return -3;

    _current_lsb = max_current_a / 32768.0f;

    // normalise to 1,2,5E+n
    uint32_t lsb_na = (uint32_t)(_current_lsb * 1e9f) + 1;
    uint32_t factor = 1;
    bool done = false;
    for (int i = 0; i < 7 && !done; i++) {
        for (int m = 1; m <= 5; m += (m == 1) ? 1 : (m == 2) ? 3 : 2) {
            if (m * factor >= lsb_na) {
                _current_lsb = (float)(m * factor) * 1e-9f;
                done = true;
                break;
            }
        }
        factor *= 10;
    }
    if (!done)
        return -4;

    uint32_t cal = (uint32_t)(0.00512f / (_current_lsb * shunt_ohm) + 0.5f);
    while (cal > 32767) {
        _current_lsb *= 2.0f;
        cal >>= 1;
    }

    // shutdown, write CAL, restart
    _write_reg(addr, REG_CONFIG, 0x0000);
    for (volatile int i = 0; i < 50000; i++)
        __asm__("nop");

    for (int retry = 0; retry < 5; retry++) {
        _write_reg(addr, REG_CALIB, (uint16_t)cal);
        for (volatile int i = 0; i < 10000; i++)
            __asm__("nop");
        if (_read_reg(addr, REG_CALIB) == (uint16_t)cal)
            break;
    }

    _write_reg(addr, REG_CONFIG, 0x4527);
    for (volatile int i = 0; i < 200000; i++)
        __asm__("nop");

    return 0;
}

float ina226_get_current_lsb(void) { return _current_lsb; }
void ina226_set_current_offset(float o) { _current_offset = o; }
float ina226_get_current_offset(void) { return _current_offset; }

// raw readings
int16_t ina226_read_shunt_voltage_raw(uint8_t a) {
    return (int16_t)_read_reg(a, REG_SHUNT_V);
}
uint16_t ina226_read_bus_voltage_raw(uint8_t a) {
    return _read_reg(a, REG_BUS_V);
}
int16_t ina226_read_current_raw(uint8_t a) {
    return (int16_t)_read_reg(a, REG_CURRENT);
}
uint16_t ina226_read_power_raw(uint8_t a) { return _read_reg(a, REG_POWER); }

// converted
float ina226_read_shunt_voltage_v(uint8_t a) {
    return (float)ina226_read_shunt_voltage_raw(a) * 2.5e-6f;
}
float ina226_read_bus_voltage_v(uint8_t a) {
    return (float)ina226_read_bus_voltage_raw(a) * 1.25e-3f;
}
float ina226_read_current_a(uint8_t a) {
    return (float)ina226_read_current_raw(a) * _current_lsb - _current_offset;
}
float ina226_read_power_w(uint8_t a) {
    return (float)ina226_read_power_raw(a) * _current_lsb * 25.0f;
}

uint16_t ina226_read_calibration(uint8_t a) { return _read_reg(a, REG_CALIB); }
uint16_t ina226_read_config(uint8_t a) { return _read_reg(a, REG_CONFIG); }
uint16_t ina226_read_mfr_id(uint8_t a) { return _read_reg(a, REG_MFR_ID); }
uint16_t ina226_read_die_id(uint8_t a) { return _read_reg(a, REG_DIE_ID); }
