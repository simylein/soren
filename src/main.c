#include "pcf8563.h"
#include "pico/stdlib.h"
#include "rp2040.h"
#include "si7021.h"
#include <stdio.h>

int main(void) {
	rp2040_stdio_init();
	rp2040_led_init();
	pcf8563_init();
	si7021_init();

	while (true) {
		rp2040_led_blink(10, 50);

		time_t time = pcf8563_time();
		float temperature = si7021_temperature();
		float humidity = si7021_humidity();
		printf("time %02d:%02d:%02d temperature %.2f humidity %.2f\n", time.hour, time.minute, time.second, temperature, humidity);

		sleep_ms(10000);
	}
}
