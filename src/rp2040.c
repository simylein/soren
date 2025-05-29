#include "pico/stdlib.h"
#include <stdio.h>

const uint rp2040_led_pin = 25;

void rp2040_stdio_init(void) {
	printf("rp2040: initialising stdio\n");
	stdio_init_all();
	sleep_ms(2000);
}

void rp2040_led_init(void) {
	printf("rp2040: initialising gpio %d\n", rp2040_led_pin);
	gpio_init(rp2040_led_pin);
	gpio_set_dir(rp2040_led_pin, GPIO_OUT);
}

void rp2040_led_blink(uint amount, uint32_t ms) {
	for (uint blinks = 0; blinks < amount; blinks++) {
		gpio_put(rp2040_led_pin, 1);
		sleep_ms(ms);
		gpio_put(rp2040_led_pin, 0);
		sleep_ms(ms);
	}
}
