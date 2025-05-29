#pragma once

#include <pico/stdlib.h>

extern const uint pcf8563_int_pin;

typedef struct rtc_t {
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
} rtc_t;

void pcf8563_init(void);
rtc_t pcf8563_time(void);
void pcf8563_alarm(uint8_t minutes);
