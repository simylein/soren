#include "pcf8563.h"
#include <hardware/i2c.h>
#include <pico/stdlib.h>
#include <stdio.h>

i2c_inst_t *pcf8563_i2c = i2c0;

const uint pcf8563_sda_pin = 0;
const uint pcf8563_scl_pin = 1;
const uint pcf8563_int_pin = 4;

const uint8_t pcf8563_addr = 0x51;

uint8_t bcd_to_dec(uint8_t val) { return ((val >> 4) * 10) + (val & 0x0f); }

uint8_t dec_to_bcd(uint8_t val) { return ((val / 10) << 4) | (val % 10); }

void pcf8563_init(void) {
	printf("pcf8563: initialising gpio %d %d and %d\n", pcf8563_sda_pin, pcf8563_scl_pin, pcf8563_int_pin);
	printf("pcf8563: initialising address 0x%x\n", pcf8563_addr);

	i2c_init(pcf8563_i2c, 100 * 1000);

	gpio_set_function(pcf8563_sda_pin, GPIO_FUNC_I2C);
	gpio_set_function(pcf8563_scl_pin, GPIO_FUNC_I2C);

	gpio_init(pcf8563_int_pin);

	gpio_set_dir(pcf8563_int_pin, GPIO_IN);

	gpio_pull_up(pcf8563_sda_pin);
	gpio_pull_up(pcf8563_scl_pin);
	gpio_pull_up(pcf8563_int_pin);
}

rtc_t pcf8563_time(void) {
	uint8_t reg = 0x02;
	uint8_t data[3];
	rtc_t time;

	if (i2c_write_blocking(pcf8563_i2c, pcf8563_addr, &reg, sizeof(reg), true) != sizeof(reg)) {
		printf("pcf8563: failed to write time register\n");
		return time;
	}

	if (i2c_read_blocking(pcf8563_i2c, pcf8563_addr, data, sizeof(data), false) != sizeof(data)) {
		printf("pcf8563: failed to read time\n");
		return time;
	}

	time.second = bcd_to_dec(data[0] & 0x7f);
	time.minute = bcd_to_dec(data[1] & 0x7f);
	time.hour = bcd_to_dec(data[2] & 0x3f);

	return time;
}

void pcf8563_alarm_schedule(uint8_t minutes) {
	uint8_t time_reg = 0x03;
	uint8_t time_data[1];
	if (i2c_write_blocking(pcf8563_i2c, pcf8563_addr, &time_reg, sizeof(time_reg), true) != sizeof(time_reg) ||
			i2c_read_blocking(pcf8563_i2c, pcf8563_addr, time_data, sizeof(time_data), false) != sizeof(time_data)) {
		printf("pcf8563: failed to read current time\n");
		return;
	}
	uint8_t minute_now = time_data[0] & 0x7f;
	uint8_t minute_next = (bcd_to_dec(minute_now) + minutes) % 60;

	uint8_t alarm_reg = 0x09;
	uint8_t alarm_data[5] = {alarm_reg, dec_to_bcd(minute_next) & 0x7f, 0x80, 0x80, 0x80};
	if (i2c_write_blocking(pcf8563_i2c, pcf8563_addr, alarm_data, sizeof(alarm_data), false) != sizeof(alarm_data)) {
		printf("pcf8563: failed to write alarm time\n");
		return;
	}

	uint8_t ctrl2_reg = 0x01;
	uint8_t ctrl2_data[2] = {ctrl2_reg, 0x02};
	if (i2c_write_blocking(pcf8563_i2c, pcf8563_addr, ctrl2_data, sizeof(ctrl2_data), false) != sizeof(ctrl2_data)) {
		printf("pcf8563: failed to enable alarm interrupt\n");
		return;
	}
}

void pcf8563_alarm_clear(void) {
	uint8_t reg = 0x01;
	uint8_t val;
	if (i2c_write_blocking(pcf8563_i2c, pcf8563_addr, &reg, sizeof(reg), true) != sizeof(reg) ||
			i2c_read_blocking(pcf8563_i2c, pcf8563_addr, &val, sizeof(val), false) != sizeof(val)) {
		printf("pcf8563: failed to read alarm register\n");
		return;
	}

	val &= ~(1 << 3);
	val |= (1 << 1);

	uint8_t data[2] = {reg, val};
	if (i2c_write_blocking(pcf8563_i2c, pcf8563_addr, data, sizeof(data), false) != sizeof(data)) {
		printf("pcf8563: failed to clear alarm interrupt\n");
		return;
	}
}
