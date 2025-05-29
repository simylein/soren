#pragma once

#include <pico/stdlib.h>

typedef struct time_t {
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
} time_t;

void pcf8563_init(void);
time_t pcf8563_time(void);
