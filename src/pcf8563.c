#include "pcf8563.h"
#include "hardware/i2c.h"
#include "pico/stdlib.h"
#include <stdio.h>

i2c_inst_t *pcf8563_i2c = i2c0;

const uint pcf8563_sda_pin = 0;
const uint pcf8563_scl_pin = 1;

const uint8_t pcf8563_addr = 0x51;

uint8_t bcd_to_dec(uint8_t val) { return ((val >> 4) * 10) + (val & 0x0f); }

void pcf8563_init(void) {
	printf("pcf8563: initialising gpio %d and %d\n", pcf8563_sda_pin, pcf8563_scl_pin);
	printf("pcf8563: initialising address 0x%x\n", pcf8563_addr);
	i2c_init(pcf8563_i2c, 100 * 1000);
	gpio_set_function(pcf8563_sda_pin, GPIO_FUNC_I2C);
	gpio_set_function(pcf8563_scl_pin, GPIO_FUNC_I2C);
	gpio_pull_up(pcf8563_sda_pin);
	gpio_pull_up(pcf8563_scl_pin);
}

rtc_t pcf8563_time(void) {
	uint8_t cmd = 0x02;
	uint8_t data[3];
	rtc_t time;

	if (i2c_write_blocking(pcf8563_i2c, pcf8563_addr, &cmd, sizeof(cmd), true) != sizeof(cmd)) {
		printf("pcf8563: failed to write time command\n");
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
