#include "config.h"
#include "logger.h"
#include <hardware/i2c.h>
#include <pico/stdlib.h>

void si7021_init(void) {
	trace("si7021 init gpio %d and %d\n", si7021_pin_sda, si7021_pin_scl);
	trace("si7021 init address 0x%02x\n", si7021_addr);

	i2c_init(si7021_i2c_inst, si7021_i2c_speed);

	gpio_set_function(si7021_pin_sda, GPIO_FUNC_I2C);
	gpio_set_function(si7021_pin_scl, GPIO_FUNC_I2C);

	gpio_pull_up(si7021_pin_sda);
	gpio_pull_up(si7021_pin_scl);
}

int si7021_temperature(uint16_t *temperature, uint32_t timeout_ms) {
	uint8_t reg = 0xf3;
	uint8_t data[3];

	trace("si7021 write reg 0x%02x\n", reg);
	if (i2c_write_blocking(si7021_i2c_inst, si7021_addr, &reg, sizeof(reg), false) != sizeof(reg)) {
		return -1;
	}

	absolute_time_t deadline = make_timeout_time_ms(timeout_ms);
	while (!time_reached(deadline)) {
		if (i2c_read_blocking(si7021_i2c_inst, si7021_addr, data, sizeof(data), false) == sizeof(data)) {
			trace("si7021 read data 0x%02x%02x\n", data[0], data[1]);
			*temperature = (data[0] << 8) | data[1];
			if ((*temperature & 0x0003) != 0x00) {
				return -1;
			}
			return 0;
		}
		sleep_us(500);
	}

	return -1;
}

float si7021_temperature_human(uint16_t temperature) { return ((175.72 * temperature) / 65536.0) - 46.85; }

int si7021_humidity(uint16_t *humidity, uint32_t timeout_ms) {
	uint8_t reg = 0xf5;
	uint8_t data[3];

	trace("si7021 write reg 0x%02x\n", reg);
	if (i2c_write_blocking(si7021_i2c_inst, si7021_addr, &reg, sizeof(reg), false) != sizeof(reg)) {
		return -1;
	}

	absolute_time_t deadline = make_timeout_time_ms(timeout_ms);
	while (!time_reached(deadline)) {
		if (i2c_read_blocking(si7021_i2c_inst, si7021_addr, data, sizeof(data), false) == sizeof(data)) {
			trace("si7021 read data 0x%02x%02x\n", data[0], data[1]);
			*humidity = (data[0] << 8) | data[1];
			if ((*humidity & 0x0003) != 0x02) {
				return -1;
			}
			return 0;
		}
		sleep_us(500);
	}

	return -1;
}

float si7021_humidity_human(uint16_t humidity) { return ((125.0 * humidity) / 65536.0) - 6.0; }
