#ifndef SENSORS_H
#define SENSORS_H
#include <stdbool.h>
#include <stdint.h>

void sensors_init(void);
void sensors_ina226_tick(uint32_t now); // sample INA226 + coulomb integration
void sensors_sht30_trigger(void);       // kick off measurement
void sensors_sht30_poll(void);          // non-blocking state machine poll

// getters
float sensors_voltage(void);
float sensors_current(void);
float sensors_power(void);
float sensors_coulomb_mah(void);
float sensors_temp(void);
float sensors_humidity(void);
bool sensors_ina_ok(void);
bool sensors_sht_ok(void);
bool sensors_current_valid(void); /* false when in deadband */

#endif
