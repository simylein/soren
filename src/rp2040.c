#include "config.h"
#include "hardware/adc.h"
#include "logger.h"
#include <pico/stdlib.h>

void sort(uint16_t *items, uint8_t items_len) {
	for (uint8_t index = 0; index < items_len - 1; index++) {
		for (uint8_t ind = 0; ind < items_len - index - 1; ind++) {
			if (items[ind] > items[ind + 1]) {
				uint16_t store = items[ind];
				items[ind] = items[ind + 1];
				items[ind + 1] = store;
			}
		}
	}
}

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

void rp2040_photovoltaic(uint16_t *photovoltaic) {
	gpio_put(rp2040_en_photovoltaic, 1);

	adc_select_input(rp2040_adc_photovoltaic);
	adc_read();

	uint16_t samples[5];
	for (uint8_t index = 0; index < sizeof(samples) / sizeof(uint16_t); index++) {
		samples[index] = adc_read();
	}
	sort(samples, sizeof(samples) / sizeof(uint16_t));
	*photovoltaic = samples[sizeof(samples) / sizeof(uint16_t) / 2];

	gpio_put(rp2040_en_photovoltaic, 0);
}

float rp2040_photovoltaic_human(uint16_t photovoltaic) { return (photovoltaic * 3.3f) / 4095.0f; }

void rp2040_battery(uint16_t *battery) {
	gpio_put(rp2040_en_battery, 1);

	adc_select_input(rp2040_adc_battery);
	adc_read();

	uint16_t samples[5];
	for (uint8_t index = 0; index < sizeof(samples) / sizeof(uint16_t); index++) {
		samples[index] = adc_read();
	}
	sort(samples, sizeof(samples) / sizeof(uint16_t));
	*battery = samples[sizeof(samples) / sizeof(uint16_t) / 2];

	gpio_put(rp2040_en_battery, 0);
}

float rp2040_battery_human(uint16_t battery) { return (battery * 3.3f) / 4095.0f; }
