
#ifndef INA226_H
#define INA226_H
#include <stdbool.h>
#include <stdint.h>

#define INA226_ADDR 0x40 // 7-bit

void ina226_init(void);
bool ina226_is_connected(uint8_t addr);
int ina226_configure(uint8_t addr, float shunt_ohm, float max_current_a);

int16_t ina226_read_shunt_voltage_raw(uint8_t addr);
uint16_t ina226_read_bus_voltage_raw(uint8_t addr);
int16_t ina226_read_current_raw(uint8_t addr);
uint16_t ina226_read_power_raw(uint8_t addr);

float ina226_read_shunt_voltage_v(uint8_t addr);
float ina226_read_bus_voltage_v(uint8_t addr);
float ina226_read_current_a(uint8_t addr);
float ina226_read_power_w(uint8_t addr);

uint16_t ina226_read_calibration(uint8_t addr);
uint16_t ina226_read_config(uint8_t addr);
uint16_t ina226_read_mfr_id(uint8_t addr);
uint16_t ina226_read_die_id(uint8_t addr);

float ina226_get_current_lsb(void);
void ina226_set_current_offset(float offset_a);
float ina226_get_current_offset(void);

#endif
