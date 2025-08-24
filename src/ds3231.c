#include "ds3231.h"
#include "config.h"
#include "logger.h"
#include <pico/stdlib.h>

const uint8_t reg_alarm = 0x07;
const uint8_t reg_ctrl = 0x0e;
const uint8_t reg_status = 0x0f;

uint8_t bin2bcd(uint8_t value) { return (uint8_t)(((value / 10) << 4) | (value % 10)); }
uint8_t bcd2bin(uint8_t value) { return (uint8_t)((value >> 4) * 10 + (value & 0x0f)); }

void ds3231_init(void) {
	trace("ds3231 init gpio %d %d and %d\n", ds3231_pin_sda, ds3231_pin_scl, ds3231_pin_int);
	trace("ds3231 init address 0x%02x\n", ds3231_addr);

	i2c_init(ds3231_i2c_inst, ds3231_i2c_speed);

	gpio_set_function(ds3231_pin_sda, GPIO_FUNC_I2C);
	gpio_set_function(ds3231_pin_scl, GPIO_FUNC_I2C);

	gpio_init(ds3231_pin_int);

	gpio_set_dir(ds3231_pin_int, GPIO_IN);

	gpio_pull_up(ds3231_pin_sda);
	gpio_pull_up(ds3231_pin_scl);
	gpio_pull_up(ds3231_pin_int);
}

int ds3231_read_register(uint8_t reg, uint8_t *data, uint8_t data_len) {
	if (i2c_write_blocking(ds3231_i2c_inst, ds3231_addr, &reg, sizeof(reg), true) != sizeof(reg)) {
		return -1;
	}

	if (i2c_read_blocking(ds3231_i2c_inst, ds3231_addr, data, data_len, false) != data_len) {
		return -1;
	};

	return 0;
}

int ds3231_write_register(uint8_t reg, uint8_t *data, uint8_t data_len) {
	uint8_t buffer[1 + data_len];
	buffer[0] = reg;
	for (uint8_t index = 0; index < data_len; index++) {
		buffer[1 + index] = data[index];
	}

	if (i2c_write_blocking(ds3231_i2c_inst, ds3231_addr, buffer, 1 + data_len, false) != 1 + data_len) {
		return -1;
	};

	return 0;
}

int ds3231_rtc(rtc_t *rtc) {
	uint8_t reg = 0x00;
	uint8_t data[3];

	if (ds3231_read_register(reg, data, sizeof(data)) == -1) {
		return -1;
	}
	trace("ds3231 read data 0x%02x%02x%02x\n", data[0], data[1], data[2]);

	rtc->hours = bcd2bin((data[2] & 0x3f));
	rtc->minutes = bcd2bin(data[1] & 0x7f);
	rtc->seconds = bcd2bin(data[0] & 0x7f);

	return 0;
}

int ds3231_alarm(uint32_t seconds) {
	uint8_t ctrl;
	if (ds3231_read_register(reg_ctrl, &ctrl, sizeof(ctrl)) == -1) {
		return -1;
	}
	trace("ds3231 read data 0x%02x\n", ctrl);

	uint8_t status;
	if (ds3231_read_register(reg_status, &status, sizeof(status)) == -1) {
		return -1;
	}
	trace("ds3231 read data 0x%02x\n", status);

	ctrl |= (uint8_t)(1u << 2 | 1u << 0);
	ctrl &= (uint8_t)~(1u << 1);
	ctrl &= (uint8_t)~(1u << 6);
	status &= (uint8_t)~(1u << 0);

	trace("ds3231 write data 0x%02x\n", ctrl);
	if (ds3231_write_register(reg_ctrl, &ctrl, sizeof(ctrl)) == -1) {
		return -1;
	}

	trace("ds3231 write data 0x%02x\n", status);
	if (ds3231_write_register(reg_status, &status, sizeof(status)) == -1) {
		return -1;
	}

	rtc_t now;
	if (ds3231_rtc(&now) == -1) {
		return -1;
	}

	uint32_t now_seconds = now.hours * 3600u + now.minutes * 60u + now.seconds;
	uint32_t today_seconds = (now_seconds + seconds) % 86400u;

	uint8_t alarm[4];
	alarm[0] = (uint8_t)(bin2bcd((uint8_t)(today_seconds % 60u)) & 0x7f);
	alarm[1] = (uint8_t)(bin2bcd((uint8_t)((today_seconds / 60u) % 60u)) & 0x7f);
	alarm[2] = (uint8_t)(bin2bcd((uint8_t)(today_seconds / 3600u)) & 0x3f);
	alarm[3] = (uint8_t)((1u << 7) | 0x01);

	trace("ds3231 write data 0x%02x%02x%02x%02x\n", alarm[0], alarm[1], alarm[2], alarm[3]);
	if (ds3231_write_register(reg_alarm, alarm, sizeof(alarm)) == -1) {
		return -1;
	}

	return 0;
}
