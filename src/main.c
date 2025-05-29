#include "pcf8563.h"
#include "rp2040.h"
#include "si7021.h"
#include <pico/sleep.h>
#include <pico/stdlib.h>
#include <stdio.h>

int main(void) {
	rp2040_stdio_init();
	rp2040_led_init();
	pcf8563_init();
	si7021_init();

	while (true) {
		rp2040_led_blink(10, 50);

		rtc_t time = pcf8563_time();
		float temperature = si7021_temperature();
		float humidity = si7021_humidity();
		printf("time %02d:%02d:%02d temperature %.2f humidity %.2f\n", time.hour, time.minute, time.second, temperature, humidity);

		pcf8563_alarm_schedule(1);
		printf("entering dormant sleep\n");
		// sleep_run_from_xosc();
		sleep_goto_dormant_until_pin(pcf8563_int_pin, true, false);
		// sleep_power_up();
		pcf8563_alarm_clear();
		printf("woke up from dormant sleep\n");
	}
}
