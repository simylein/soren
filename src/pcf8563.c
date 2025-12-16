#include "config.h"
#include "logger.h"
#include <pico/stdlib.h>
#include <pico/types.h>

uint8_t bin2bcd(uint8_t value) { return (uint8_t)(((value / 10) << 4) | (value % 10)); }
uint8_t bcd2bin(uint8_t value) { return (uint8_t)((value >> 4) * 10 + (value & 0x0f)); }

void pcf8563_init(void) {
	trace("pcf8563 init gpio %d %d and %d\n", pcf8563_pin_sda, pcf8563_pin_scl, pcf8563_pin_int);
	trace("pcf8563 init address 0x%02x\n", pcf8563_addr);

	i2c_init(pcf8563_i2c_inst, pcf8563_i2c_speed);

	gpio_set_function(pcf8563_pin_sda, GPIO_FUNC_I2C);
	gpio_set_function(pcf8563_pin_scl, GPIO_FUNC_I2C);

	gpio_init(pcf8563_pin_int);

	gpio_set_dir(pcf8563_pin_int, GPIO_IN);

	gpio_pull_up(pcf8563_pin_sda);
	gpio_pull_up(pcf8563_pin_scl);
	gpio_pull_up(pcf8563_pin_int);
}

int pcf8563_read_register(uint8_t reg, uint8_t *data, uint8_t data_len) {
	if (i2c_write_blocking(pcf8563_i2c_inst, pcf8563_addr, &reg, sizeof(reg), true) != sizeof(reg)) {
		return -1;
	}

	if (i2c_read_blocking(pcf8563_i2c_inst, pcf8563_addr, data, data_len, false) != data_len) {
		return -1;
	};

	return 0;
}

int pcf8563_write_register(uint8_t reg, uint8_t *data, uint8_t data_len) {
	uint8_t buffer[1 + data_len];
	buffer[0] = reg;
	for (uint8_t index = 0; index < data_len; index++) {
		buffer[1 + index] = data[index];
	}

	if (i2c_write_blocking(pcf8563_i2c_inst, pcf8563_addr, buffer, 1 + data_len, false) != 1 + data_len) {
		return -1;
	};

	return 0;
}

int pcf8563_datetime(datetime_t *datetime) {
	uint8_t reg = 0x02;
	uint8_t data[7];

	if (pcf8563_read_register(reg, data, sizeof(data)) == -1) {
		return -1;
	}
	trace("pcf8563 read data 0x%02x%02x%02x%02x%02x%02x%02x\n", data[0], data[1], data[2], data[3], data[4], data[5], data[6]);

	datetime->sec = bcd2bin(data[0] & 0x7f);
	datetime->min = bcd2bin(data[1] & 0x7f);
	datetime->hour = bcd2bin((data[2] & 0x3f));

	datetime->day = bcd2bin(data[3] & 0x3f);
	datetime->dotw = data[4] & 0x07;
	datetime->month = bcd2bin(data[5] & 0x1f);

	datetime->year = bcd2bin(data[6]) + ((data[5] & 0x80) ? 1900 : 2000);

	return 0;
}

int pcf8563_alarm(uint8_t ticks) {
	uint8_t timer_ctrl = 0x01;
	if (pcf8563_write_register(0x0e, &timer_ctrl, sizeof(timer_ctrl)) == -1) {
		return -1;
	}
	trace("pcf8563 write data 0x%02x\n", timer_ctrl);

	if (pcf8563_write_register(0x0f, &ticks, sizeof(ticks)) == -1) {
		return -1;
	}
	trace("pcf8563 write data 0x%02x\n", ticks);

	uint8_t cs2;
	if (pcf8563_read_register(0x01, &cs2, sizeof(cs2)) == -1) {
		return -1;
	}
	trace("pcf8563 read data 0x%02x\n", cs2);

	uint8_t new_cs2 = 0;
	new_cs2 |= (cs2 & (1u << 1));
	new_cs2 |= (1u << 0);
	if (pcf8563_write_register(0x01, &new_cs2, sizeof(new_cs2)) == -1) {
		return -1;
	}
	trace("pcf8563 write data 0x%02x\n", new_cs2);

	timer_ctrl |= 0x80;
	if (pcf8563_write_register(0x0e, &timer_ctrl, sizeof(timer_ctrl)) == -1) {
		return -1;
	}
	trace("pcf8563 write data 0x%02x\n", timer_ctrl);

	return 0;
}

int pcf8563_alarm_clear(void) {
	uint8_t cs2;
	if (pcf8563_read_register(0x01, &cs2, sizeof(cs2)) == -1) {
		return -1;
	}
	trace("pcf8563 read data 0x%02x\n", cs2);

	cs2 &= ~(1u << 2);
	if (pcf8563_write_register(0x01, &cs2, sizeof(cs2)) == -1) {
		return -1;
	}
	trace("pcf8563 write data 0x%02x\n", cs2);

	return 0;
}
