#pragma once

#include <stdint.h>

void si7021_init(void);

int si7021_temperature(uint16_t *temperature);
float si7021_temperature_human(uint16_t temperature);
int si7021_humidity(uint16_t *humidity);
float si7021_humidity_human(uint16_t humidity);
