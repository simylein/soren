#pragma once

#include <stdint.h>

void ads1115_init(void);

int ads1115_photovoltaic(int16_t *photovoltaic);
float ads1115_photovoltaic_human(int16_t photovoltaic);
int ads1115_battery(int16_t *battery);
float ads1115_battery_human(int16_t battery);
