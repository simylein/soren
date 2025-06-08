#pragma once

void rp2040_stdio_init(void);
void rp2040_stdio_deinit(void);

void rp2040_adc_init();

void rp2040_led_init(void);
void rp2040_led_blink(uint amount, uint8_t ms, bool cooloff);

float rp2040_photovoltaic();
float rp2040_battery();
