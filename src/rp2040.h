#pragma once

void rp2040_stdio_init(void);
void rp2040_stdio_deinit(void);

void rp2040_led_init(void);
void rp2040_led_set(int value);
void rp2040_led_blink(uint amount, uint32_t ms);
