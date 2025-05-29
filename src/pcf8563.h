#pragma once

#include <pico/stdlib.h>

typedef struct rtc_t {
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
} rtc_t;

void pcf8563_init(void);
rtc_t pcf8563_time(void);
