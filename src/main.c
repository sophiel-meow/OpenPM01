#include "ui.h"
#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/rcc.h>

volatile uint32_t sys_ticks;

void sys_tick_handler(void) __attribute__((used));
void sys_tick_handler(void) { sys_ticks++; }

int main(void) {
    rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE8_72MHZ]);
    systick_set_reload(rcc_ahb_frequency / 1000 - 1);
    systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
    systick_counter_enable();
    systick_interrupt_enable();

    ui_init();
    sensors_init();

    uint32_t last_ina = sys_ticks;
    uint32_t last_sht = sys_ticks;
    uint32_t last_ui = sys_ticks;

    while ("meow") {
        uint32_t now = sys_ticks;

        if (now - last_ina >= INA226_PERIOD_MS) {
            last_ina = now;
            sensors_ina226_tick(now);
        }

        if (now - last_sht >= SHT30_PERIOD_MS) {
            last_sht = now;
            sensors_sht30_trigger();
        }

        sensors_sht30_poll();

        if (now - last_ui >= UI_PERIOD_MS) {
            last_ui = now;
            ui_draw();
        }
    }

    return 0;
}
