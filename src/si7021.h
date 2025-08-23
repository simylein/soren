#pragma once

#include <stdint.h>

void si7021_init(void);

int si7021_temperature(uint16_t *temperature, uint32_t timeout_ms);
float si7021_temperature_human(uint16_t temperature);
int si7021_humidity(uint16_t *humidity, uint32_t timeout_ms);
float si7021_humidity_human(uint16_t humidity);
