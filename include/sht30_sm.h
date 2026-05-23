#ifndef SHT30_SM_H
#define SHT30_SM_H
#include <stdbool.h>
#include <stdint.h>

void sht30_sm_init(void);
void sht30_sm_start(uint8_t addr);
int sht30_sm_poll(void); /* returns 1 when new data ready */
void sht30_sm_get(float *temp_c, float *hum_pct);
bool sht30_sm_ok(void);

#endif
