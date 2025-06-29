#include <hardware/adc.h>
#include <hardware/clocks.h>
#include <pico/stdlib.h>
#include <stdio.h>
#include <tusb.h>

const bool rp2040_debug = false;

const uint rp2040_led_pin = 25;

const uint rp2040_photovoltaic_adc = 0;
const uint rp2040_photovoltaic_pin = 26;
const uint rp2040_battery_adc = 1;
const uint rp2040_battery_pin = 27;

void rp2040_stdio_init(void) {
	if (rp2040_debug) {
		printf("rp2040: initialising stdio\n");
	}

	stdio_init_all();
	sleep_ms(2000);
}

void rp2040_stdio_deinit() {
	clock_stop(clk_usb);
	tud_disconnect();
}

void rp2040_led_init(void) {
	if (rp2040_debug) {
		printf("rp2040: initialising gpio %d\n", rp2040_led_pin);
	}

	gpio_init(rp2040_led_pin);
	gpio_set_dir(rp2040_led_pin, GPIO_OUT);
}

void rp2040_adc_init() {
	printf("si7021: initialising gpio %d and %d\n", rp2040_photovoltaic_pin, rp2040_battery_pin);

	adc_init();

	adc_gpio_init(rp2040_photovoltaic_pin);
	adc_gpio_init(rp2040_battery_pin);
}

void rp2040_led_blink(uint amount, uint8_t ms, bool cooloff) {
	for (uint blinks = 0; blinks < amount; blinks++) {
		gpio_put(rp2040_led_pin, 1);
		sleep_ms(ms);
		gpio_put(rp2040_led_pin, 0);
		sleep_ms(ms);
	}
	if (cooloff) {
		sleep_ms(ms * 4);
	}
}

float rp2040_photovoltaic() {
	adc_select_input(rp2040_photovoltaic_adc);

	uint16_t raw_photovoltaic = adc_read();
	float photovoltaic = (raw_photovoltaic * 3.3f) / 4095.0f;
	return photovoltaic;
}

float rp2040_battery() {
	adc_select_input(rp2040_battery_adc);

	uint16_t raw_battery = adc_read();
	float battery = (raw_battery * 3.3f) / 4095.0f;
	return battery;
}
