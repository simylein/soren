#pragma once

#include <pico/types.h>
#include <stdint.h>

uint8_t bin2bcd(uint8_t value);
uint8_t bcd2bin(uint8_t value);

void pcf8563_init(void);

int pcf8563_datetime(datetime_t *datetime);
int pcf8563_alarm(uint8_t ticks);
int pcf8563_alarm_clear(void);
