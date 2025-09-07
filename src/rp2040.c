#include "config.h"
#include "hardware/adc.h"
#include "logger.h"
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

void rp2040_photovoltaic(uint16_t *photovoltaic) {
	gpio_put(rp2040_en_photovoltaic, 1);

	adc_select_input(rp2040_adc_photovoltaic);

	adc_read();
	*photovoltaic = adc_read();

	gpio_put(rp2040_en_photovoltaic, 0);
}

float rp2040_photovoltaic_human(uint16_t photovoltaic) { return (photovoltaic * 3.3) / 4095.0; }

void rp2040_battery(uint16_t *battery) {
	gpio_put(rp2040_en_battery, 1);

	adc_select_input(rp2040_adc_battery);

	adc_read();
	*battery = adc_read();

	gpio_put(rp2040_en_battery, 0);
}

float rp2040_battery_human(uint16_t battery) { return (battery * 3.3) / 4095.0; }
