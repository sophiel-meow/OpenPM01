#include "sht30_sm.h"
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>

/*
** software I2C
** - SDA: PB8
** - SCL: PB9
*/
#define I2C_PORT GPIOB
#define I2C_SDA GPIO8
#define I2C_SCL GPIO9
#define SDA_H GPIO_BSRR(I2C_PORT) = I2C_SDA
#define SDA_L GPIO_BRR(I2C_PORT) = I2C_SDA
#define SCL_H GPIO_BSRR(I2C_PORT) = I2C_SCL
#define SCL_L GPIO_BRR(I2C_PORT) = I2C_SCL
#define SDA_IN (GPIO_IDR(I2C_PORT) & I2C_SDA)
#define SCL_IN (GPIO_IDR(I2C_PORT) & I2C_SCL)

static void _hbit(void) {
    for (volatile int i = 0; i < 10; i++)
        __asm__("nop");
}

static void _scl_release(void) {
    volatile uint32_t to = 100000;
    SCL_H;
    while (!SCL_IN && --to)
        ;
    _hbit();
}

static void _start(void) {
    SDA_H;
    _scl_release();
    SDA_L;
    _hbit();
    SCL_L;
}
static void _stop(void) {
    SDA_L;
    _scl_release();
    SDA_H;
    _hbit();
}

static int _write(uint8_t b) {
    for (int i = 0; i < 8; i++) {
        if (b & 0x80)
            SDA_H;
        else
            SDA_L;
        _hbit();
        _scl_release();
        SCL_L;
        b <<= 1;
    }
    SDA_H;
    _hbit();
    _scl_release();
    int ack = SDA_IN ? 1 : 0;
    SCL_L;
    return ack;
}

static uint8_t _read(int ack) {
    uint8_t b = 0;
    SDA_H;
    for (int i = 0; i < 8; i++) {
        _scl_release();
        b = (b << 1) | (SDA_IN ? 1 : 0);
        SCL_L;
        _hbit();
    }
    if (ack)
        SDA_L;
    else
        SDA_H;
    _hbit();
    _scl_release();
    SCL_L;
    SDA_H;
    return b;
}

// CRC-8 poly 0x31, init 0xFF
static uint8_t _crc8(const uint8_t *d, int n) {
    uint8_t c = 0xFF;
    for (int j = 0; j < n; j++) {
        c ^= d[j];
        for (int i = 0; i < 8; i++)
            c = (c & 0x80) ? (c << 1) ^ 0x31 : (c << 1);
    }
    return c;
}

// state machine
#define SHT30_CMD_MEASURE 0x2400
#define WAIT_MS 20

static enum { SM_IDLE, SM_WAIT, SM_READ } g_state = SM_IDLE;
static uint32_t g_wait_start; // sys_ticks when wait began
static float g_result_t, g_result_h;
static bool g_result_ok;
static uint8_t g_dev_addr; // 8-bit address

extern volatile uint32_t sys_ticks;

/* public */
void sht30_sm_init(void) {
    rcc_periph_clock_enable(RCC_GPIOB);
    gpio_set_mode(I2C_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_OPENDRAIN,
                  I2C_SDA | I2C_SCL);
    gpio_set(I2C_PORT, I2C_SDA | I2C_SCL);
}

void sht30_sm_start(uint8_t addr) {
    if (g_state != SM_IDLE)
        return; // still busy

    // send measurement command
    _start();
    if (_write(addr))
        goto fail;
    if (_write(0x24))
        goto fail; // SHT30_CMD_MEASURE_HIGH >> 8
    if (_write(0x00))
        goto fail; // SHT30_CMD_MEASURE_HIGH & 0xFF
    _stop();

    g_dev_addr = addr;
    g_wait_start = sys_ticks;
    g_state = SM_WAIT;
    return;

fail:
    _stop();
    g_state = SM_IDLE;
    g_result_ok = false;
}

int sht30_sm_poll(void) {

    if (g_state == SM_WAIT) {
        if (sys_ticks - g_wait_start < WAIT_MS)
            return 0;
        // time to read
        g_state = SM_READ;
        uint8_t data[6];

        _start();
        if (_write(g_dev_addr | 1)) {
            _stop();
            goto done;
        }
        for (int i = 0; i < 6; i++)
            data[i] = _read(i < 5);
        _stop();

        if (_crc8(data, 2) != data[2] || _crc8(data + 3, 2) != data[5])
            goto done;

        uint16_t tr = ((uint16_t)data[0] << 8) | data[1];
        uint16_t hr = ((uint16_t)data[3] << 8) | data[4];
        g_result_t = (float)tr / 65535.0f * 175.0f - 45.0f;
        g_result_h = (float)hr / 65535.0f * 100.0f;
        g_result_ok = true;
        g_state = SM_IDLE;
        return 1;
    done:
        g_result_ok = false;
        g_state = SM_IDLE;
        return 1;
    }
    return 0;
}

void sht30_sm_get(float *t, float *h) {
    *t = g_result_t;
    *h = g_result_h;
}
bool sht30_sm_ok(void) { return g_result_ok; }
