#include "config.h"
#include "hardware/clocks.h"
#include "logger.h"
#include "sleep.h"
#include <hardware/adc.h>
#include <hardware/rtc.h>
#include <pico/stdlib.h>

void rp2040_clocks(uint32_t frequency) {
	uint32_t src_frequency = clock_get_hz(clk_sys);

	clock_configure(clk_sys, CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLKSRC_CLK_SYS_AUX, CLOCKS_CLK_SYS_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS,
									src_frequency, frequency);
}

void rp2040_stdio_init(void) {
	trace("rp2040 init stdio\n");

	stdio_init_all();

	sleep_ms(2000);
}

void rp2040_adc_init(void) {
	trace("rp2040 init gpio %d and %d\n", rp2040_pin_photovoltaic, rp2040_pin_battery);

	adc_init();
	adc_set_clkdiv(128);

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
		sleep(100);
		gpio_put(rp2040_pin_led, 0);
		sleep(100);
	}
	sleep(200);
}

void rp2040_photovoltaic(uint16_t *photovoltaic, uint8_t samples) {
	gpio_put(rp2040_en_photovoltaic, 1);

	sleep_ms(5);

	adc_select_input(rp2040_adc_photovoltaic);
	adc_read();

	uint32_t accumulator = 0;
	for (uint8_t index = 0; index < samples; index++) {
		accumulator += adc_read();
	}
	*photovoltaic = accumulator / samples;

	gpio_put(rp2040_en_photovoltaic, 0);
}

float rp2040_photovoltaic_human(uint16_t photovoltaic) { return (photovoltaic * 3.3f) / 4095.0f; }

void rp2040_battery(uint16_t *battery, uint8_t samples) {
	gpio_put(rp2040_en_battery, 1);

	sleep_ms(5);

	adc_select_input(rp2040_adc_battery);
	adc_read();

	uint32_t accumulator = 0;
	for (uint8_t index = 0; index < samples; index++) {
		accumulator += adc_read();
	}
	*battery = accumulator / samples;

	gpio_put(rp2040_en_battery, 0);
}

float rp2040_battery_human(uint16_t battery) { return (battery * 3.3f) / 4095.0f; }
