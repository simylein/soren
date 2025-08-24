#pragma once

#include <stdint.h>

typedef struct rtc_t {
	uint8_t hours;
	uint8_t minutes;
	uint8_t seconds;
} rtc_t;

void ds3231_init(void);

int ds3231_rtc(rtc_t *rtc);
int ds3231_alarm(uint32_t seconds);
