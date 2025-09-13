#pragma once

void rp2040_stdio_init(void);
void rp2040_adc_init(void);

void rp2040_photovoltaic(uint16_t *photovoltaic, uint8_t samples);
float rp2040_photovoltaic_human(uint16_t photovoltaic);
void rp2040_battery(uint16_t *battery, uint8_t samples);
float rp2040_battery_human(uint16_t battery);
