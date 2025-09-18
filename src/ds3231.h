#pragma once

#include <pico/types.h>
#include <stdint.h>

void ds3231_init(void);

int ds3231_datetime(datetime_t *datetime);
int ds3231_alarm(uint32_t seconds);
int ds3231_alarm_clear(void);
