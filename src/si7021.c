#include <hardware/i2c.h>
#include <pico/stdlib.h>
#include <stdio.h>

const bool si7021_debug = false;

i2c_inst_t *si7021_i2c = i2c1;

const uint si7021_sda_pin = 6;
const uint si7021_scl_pin = 7;

const uint8_t si7021_addr = 0x40;

void si7021_init(void) {
	if (si7021_debug) {
		printf("si7021: initialising gpio %d and %d\n", si7021_sda_pin, si7021_scl_pin);
		printf("si7021: initialising address 0x%x\n", si7021_addr);
	}

	i2c_init(si7021_i2c, 100 * 1000);

	gpio_set_function(si7021_sda_pin, GPIO_FUNC_I2C);
	gpio_set_function(si7021_scl_pin, GPIO_FUNC_I2C);

	gpio_pull_up(si7021_sda_pin);
	gpio_pull_up(si7021_scl_pin);
}

int si7021_temperature(uint16_t *temperature) {
	uint8_t reg = 0xe3;
	uint8_t data[2];

	if (i2c_write_blocking(si7021_i2c, si7021_addr, &reg, sizeof(reg), true) != sizeof(reg)) {
		printf("si7021: failed to write temperature register\n");
		return -1;
	}

	sleep_ms(10);

	if (i2c_read_blocking(si7021_i2c, si7021_addr, data, sizeof(data), false) != sizeof(data)) {
		printf("si7021: failed to read temperature\n");
		return -1;
	}

	*temperature = (data[0] << 8) | data[1];
	return 0;
}

float si7021_temperature_human(uint16_t temperature) {
	float temperature_human = ((175.72 * temperature) / 65536.0) - 46.85;
	return temperature_human;
}

int si7021_humidity(uint16_t *humidity) {
	uint8_t reg = 0xe5;
	uint8_t data[2];

	if (i2c_write_blocking(si7021_i2c, si7021_addr, &reg, 1, true) != 1) {
		printf("si7021: failed to write humidity register\n");
		return -1;
	}

	sleep_ms(10);

	if (i2c_read_blocking(si7021_i2c, si7021_addr, data, 2, false) != 2) {
		printf("si7021: failed to read humidity\n");
		return -1;
	}

	*humidity = (data[0] << 8) | data[1];
	return 0;
}
float si7021_humidity_human(uint16_t humidity) {
	float humidity_human = ((125.0 * humidity) / 65536.0) - 6.0;
	return humidity_human;
}
