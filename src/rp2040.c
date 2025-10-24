#include "config.h"
#include "logger.h"
#include <hardware/adc.h>
#include <hardware/rtc.h>
#include <pico/stdlib.h>

void rp2040_stdio_init(void) {
	trace("rp2040 init stdio\n");

	stdio_init_all();

	sleep_ms(2048);
}

void rp2040_adc_init(void) {
	trace("rp2040 init gpio %d and %d\n", rp2040_pin_photovoltaic, rp2040_pin_battery);

	adc_init();

	adc_gpio_init(rp2040_pin_photovoltaic);
	adc_gpio_init(rp2040_pin_battery);

	gpio_set_dir(rp2040_en_photovoltaic, GPIO_OUT);
	gpio_set_dir(rp2040_en_battery, GPIO_OUT);
}

void rp2040_led_init(void) {
	trace("rp2040 init gpio %d\n", rp2040_pin_led);

	gpio_init(rp2040_pin_led);

	gpio_set_dir(rp2040_pin_led, GPIO_OUT);
}

void rp2040_led_blink(uint8_t blinks) {
	for (uint8_t ind = 0; ind < blinks; ind++) {
		gpio_put(rp2040_pin_led, 1);
		sleep_ms(64);
		gpio_put(rp2040_pin_led, 0);
		sleep_ms(64);
	}
	sleep_ms(256);
}

void rp2040_photovoltaic(uint16_t *photovoltaic, uint8_t samples) {
	gpio_put(rp2040_en_photovoltaic, 1);

	sleep_us(5000);

	adc_select_input(rp2040_adc_photovoltaic);
	adc_read();

	uint32_t accumulator = 0;
	for (uint8_t index = 0; index < samples; index++) {
		accumulator += adc_read();
	}
	*photovoltaic = accumulator / samples;

	gpio_put(rp2040_en_photovoltaic, 0);
}

float rp2040_photovoltaic_human(uint16_t photovoltaic) { return (photovoltaic * 3.3f) / 4095.0f * 2; }

void rp2040_battery(uint16_t *battery, uint8_t samples) {
	gpio_put(rp2040_en_battery, 1);

	sleep_us(5000);

	adc_select_input(rp2040_adc_battery);
	adc_read();

	uint32_t accumulator = 0;
	for (uint8_t index = 0; index < samples; index++) {
		accumulator += adc_read();
	}
	*battery = accumulator / samples;

	gpio_put(rp2040_en_battery, 0);
}

float rp2040_battery_human(uint16_t battery) { return (battery * 3.3f) / 4095.0f * 2; }
