#include "sensors.h"
#include "ina226.h"
#include "sht30_sm.h"

#define INA226_ADDR 0x40
#define SHT30_ADDR 0x88

/* private */
#define CURRENT_DEADBAND 0.005f // abs 5mA

static float g_voltage, g_current, g_power, g_coulomb;
static float g_temp, g_hum;
static bool g_ina_ok, g_sht_ok;
static bool g_current_valid = true;
static uint32_t g_ina_last_tick;

/* public */
void sensors_init(void) {
    ina226_init();
    ina226_configure(INA226_ADDR, 0.001f, 30.0f);
    // ina226_set_current_offset(-0.002f);
    g_ina_ok = ina226_is_connected(INA226_ADDR);

    sht30_sm_init();
    sht30_sm_start(SHT30_ADDR);
}

void sensors_ina226_tick(uint32_t now) {
    if (!g_ina_ok)
        return;
    float dt_s = (now - g_ina_last_tick) / 1000.0f;
    g_ina_last_tick = now;
    g_voltage = ina226_read_bus_voltage_v(INA226_ADDR);
    g_current = ina226_read_current_a(INA226_ADDR);

    // deadband: 5mA
    if (g_current < 0.0f ? -g_current < CURRENT_DEADBAND
                         : g_current < CURRENT_DEADBAND) {
        g_current = 0.0f;
        g_power = 0.0f;
        g_current_valid = false;
    } else {
        float current_abs = g_current < 0.0f ? -g_current : g_current;
        g_power = g_voltage * current_abs;
        g_coulomb += current_abs * dt_s * (1000.0f / 3600.0f);
        g_current_valid = true;
    }
}

void sensors_sht30_trigger(void) { sht30_sm_start(SHT30_ADDR); }

void sensors_sht30_poll(void) {
    if (sht30_sm_poll()) {
        sht30_sm_get(&g_temp, &g_hum);
        g_sht_ok = sht30_sm_ok();
    }
}

// getters
float sensors_voltage(void) { return g_voltage; }
float sensors_current(void) { return g_current; }
float sensors_power(void) { return g_power; }
float sensors_coulomb_mah(void) { return g_coulomb; }
float sensors_temp(void) { return g_temp; }
float sensors_humidity(void) { return g_hum; }
bool sensors_ina_ok(void) { return g_ina_ok; }
bool sensors_sht_ok(void) { return g_sht_ok; }
bool sensors_current_valid(void) { return g_current_valid; }
