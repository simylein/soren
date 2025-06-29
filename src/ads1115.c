#include <hardware/i2c.h>
#include <pico/stdlib.h>
#include <stdio.h>

const bool ads1115_debug = false;

i2c_inst_t *ads1115_i2c = i2c1;

const uint ads1115_sda_pin = 6;
const uint ads1115_scl_pin = 7;

const uint8_t ads1115_addr = 0x48;

void ads1115_init(void) {
	if (ads1115_debug) {
		printf("ads1115: initialising gpio %d and %d\n", ads1115_sda_pin, ads1115_scl_pin);
		printf("ads1115: initialising address 0x%x\n", ads1115_addr);
	}

	i2c_init(ads1115_i2c, 100 * 1000);

	gpio_set_function(ads1115_sda_pin, GPIO_FUNC_I2C);
	gpio_set_function(ads1115_scl_pin, GPIO_FUNC_I2C);

	gpio_pull_up(ads1115_sda_pin);
	gpio_pull_up(ads1115_scl_pin);
}

int ads1115_photovoltaic(int16_t *photovoltaic) {
	uint8_t reg[3] = {0x01, 0xc2, 0x83};
	uint8_t data[2];

	if (i2c_write_blocking(ads1115_i2c, ads1115_addr, reg, sizeof(reg), true) != sizeof(reg)) {
		printf("ads1115: failed to write photovoltaic register\n");
		return -1;
	}

	sleep_ms(130);

	uint8_t conv_ptr = 0x00;
	if (i2c_write_blocking(ads1115_i2c, ads1115_addr, &conv_ptr, sizeof(conv_ptr), true) != sizeof(conv_ptr)) {
		printf("ads1115: failed to write pointer conversion register\n");
		return -1;
	}

	if (i2c_read_blocking(ads1115_i2c, ads1115_addr, data, sizeof(data), false) != sizeof(data)) {
		printf("ads1115: failed to read photovoltaic\n");
		return -1;
	}

	if (ads1115_debug) {
		printf("ads1115: raw photovoltaic value %02x%02x\n", data[0], data[1]);
	}

	*photovoltaic = (data[0] << 8) | data[1];
	return 0;
}

float ads1115_photovoltaic_human(int16_t photovoltaic) {
	float photovoltaic_human = ((photovoltaic * 4.096) / 32768.0) * 1;
	return photovoltaic_human;
}

int ads1115_battery(int16_t *battery) {
	uint8_t reg[3] = {0x01, 0xc4, 0x83};
	uint8_t data[2];

	if (i2c_write_blocking(ads1115_i2c, ads1115_addr, reg, sizeof(reg), true) != sizeof(reg)) {
		printf("ads1115: failed to write battery register\n");
		return -1;
	}

	sleep_ms(130);

	uint8_t conv_ptr = 0x00;
	if (i2c_write_blocking(ads1115_i2c, ads1115_addr, &conv_ptr, sizeof(conv_ptr), true) != sizeof(conv_ptr)) {
		printf("ads1115: failed to write pointer conversion register\n");
		return -1;
	}

	if (i2c_read_blocking(ads1115_i2c, ads1115_addr, data, sizeof(data), false) != sizeof(data)) {
		printf("ads1115: failed to read battery\n");
		return -1;
	}

	if (ads1115_debug) {
		printf("ads1115: raw battery value %02x%02x\n", data[0], data[1]);
	}

	*battery = (data[0] << 8) | data[1];
	return 0;
}

float ads1115_battery_human(int16_t battery) {
	float battery_human = ((battery * 4.096) / 32768.0) * 1;
	return battery_human;
}
