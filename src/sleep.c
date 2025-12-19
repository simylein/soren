#include "config.h"
#include "logger.h"
#include "math.h"
#include "pcf8563.h"
#include "rp2040.h"
#include "si7021.h"
#include "sx1278.h"
#include <pico/sleep.h>
#include <stdint.h>

void sleep(uint32_t duration) {
	if (deep_sleep == false) {
		sleep_ms(duration);
	} else {
		while (duration > 0) {
			uint32_t milliseconds = min32(duration, 3984);
			uint8_t ticks = (milliseconds * 64 + 500) / 1000;
			if (ticks < 2) {
				ticks = 2;
			}

			if (pcf8563_alarm(ticks) == -1) {
				error("pcf8563 failed to write alarm\n");
			}

			sleep_run_from_xosc();
			sleep_goto_dormant_until_pin(pcf8563_pin_int, true, false);
			sleep_power_up();

			pcf8563_init();

			if (pcf8563_alarm_clear() == -1) {
				error("pcf8563 failed to clear alarm\n");
			}

			duration = sub32(duration, milliseconds);
		}

		si7021_init();
		sx1278_init();
		rp2040_adc_init();
	}
}
