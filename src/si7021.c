#include <hardware/i2c.h>
#include <pico/stdlib.h>
#include <stdio.h>

i2c_inst_t *si7021_i2c = i2c1;

const uint si7021_sda_pin = 2;
const uint si7021_scl_pin = 3;

const uint8_t si7021_addr = 0x40;

void si7021_init(void) {
	printf("si7021: initialising gpio %d and %d\n", si7021_sda_pin, si7021_scl_pin);
	printf("si7021: initialising address 0x%x\n", si7021_addr);
	i2c_init(si7021_i2c, 100 * 1000);
	gpio_set_function(si7021_sda_pin, GPIO_FUNC_I2C);
	gpio_set_function(si7021_scl_pin, GPIO_FUNC_I2C);
	gpio_pull_up(si7021_sda_pin);
	gpio_pull_up(si7021_scl_pin);
}

float si7021_temperature(void) {
	uint8_t cmd = 0xe3;
	uint8_t data[2];

	if (i2c_write_blocking(si7021_i2c, si7021_addr, &cmd, sizeof(cmd), true) != sizeof(cmd)) {
		printf("si7021: failed to write temperature command\n");
		return -999.0f;
	}

	sleep_ms(10);

	if (i2c_read_blocking(si7021_i2c, si7021_addr, data, sizeof(data), false) != sizeof(data)) {
		printf("si7021: failed to read temperature\n");
		return -999.0f;
	}

	uint16_t raw_temperature = (data[0] << 8) | data[1];
	float temperature = ((175.72 * raw_temperature) / 65536.0) - 46.85;
	return temperature;
}

float si7021_humidity(void) {
	uint8_t cmd = 0xe5;
	uint8_t data[2];

	if (i2c_write_blocking(si7021_i2c, si7021_addr, &cmd, 1, true) != 1) {
		printf("si7021: failed to write humidity command\n");
		return -999.0f;
	}

	sleep_ms(10);

	if (i2c_read_blocking(si7021_i2c, si7021_addr, data, 2, false) != 2) {
		printf("si7021: failed to read humidity\n");
		return -999.0f;
	}

	uint16_t raw_humidity = (data[0] << 8) | data[1];
	float humidity = ((125.0 * raw_humidity) / 65536.0) - 6.0;
	return humidity;
}
