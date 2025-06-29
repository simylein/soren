#include "ads1115.h"
#include "pcf8563.h"
#include "rp2040.h"
#include "si7021.h"
#include "sx1278.h"
#include <hardware/clocks.h>
#include <pico/sleep.h>
#include <pico/stdlib.h>
#include <stdio.h>
#include <string.h>

const uint16_t id = 0xa391;

uint16_t hton16(uint16_t value) { return (value << 8) | (value >> 8); }

int main(void) {
	rp2040_stdio_init();
	rp2040_led_init();
	pcf8563_init();
	si7021_init();
	ads1115_init();
	sx1278_init();
	sx1278_sleep();

	while (true) {
		rp2040_led_blink(1, 50, true);

		rtc_t time = pcf8563_time();
		uint16_t temperature;
		si7021_temperature(&temperature);
		uint16_t humidity;
		si7021_humidity(&humidity);
		printf("time %02d:%02d:%02d temperature %.2f humidity %.2f\n", time.hour, time.minute, time.second,
					 si7021_temperature_human(temperature), si7021_humidity_human(humidity));
		int16_t photovoltaic;
		ads1115_photovoltaic(&photovoltaic);
		int16_t battery;
		ads1115_battery(&battery);
		printf("time %02d:%02d:%02d photovoltaic %.3f battery %.3f\n", time.hour, time.minute, time.second,
					 ads1115_photovoltaic_human(photovoltaic), ads1115_battery_human(battery));

		rp2040_led_blink(2, 50, true);

		sx1278_standby();
		sx1278_frequency(433 * 1000 * 1000);
		sx1278_tx_power(2);
		sx1278_coding_rate(5);
		sx1278_bandwidth(125 * 1000);
		sx1278_spreading_factor(7);
		sx1278_checksum(true);

		uint8_t tx_data[256];
		uint8_t tx_data_len = 0;
		memcpy(&tx_data[tx_data_len], &(uint16_t[]){hton16(id)}, sizeof(id));
		tx_data_len += sizeof(id);
		uint8_t kind = 0x01;
		memcpy(&tx_data[tx_data_len], &kind, sizeof(kind));
		tx_data_len += sizeof(kind);
		memcpy(&tx_data[tx_data_len], &(uint16_t[]){hton16(temperature)}, sizeof(temperature));
		tx_data_len += sizeof(temperature);
		memcpy(&tx_data[tx_data_len], &(uint16_t[]){hton16(humidity)}, sizeof(humidity));
		tx_data_len += sizeof(humidity);
		sx1278_send(&tx_data, tx_data_len, 1000);

		rp2040_led_blink(3, 50, true);

		// uint8_t rx_data[256];
		// uint8_t rx_data_len = 0;
		// sx1278_recv(&rx_data, &rx_data_len, 1000);

		// if (rx_data_len != 0) {
		// 	rp2040_led_blink(4, 50, true);
		// }

		tx_data_len = 0;
		memcpy(&tx_data[tx_data_len], &(uint16_t[]){hton16(id)}, sizeof(id));
		tx_data_len += sizeof(id);
		kind = 0x02;
		memcpy(&tx_data[tx_data_len], &kind, sizeof(kind));
		tx_data_len += sizeof(kind);
		memcpy(&tx_data[tx_data_len], &(uint16_t[]){hton16(photovoltaic)}, sizeof(photovoltaic));
		tx_data_len += sizeof(photovoltaic);
		memcpy(&tx_data[tx_data_len], &(uint16_t[]){hton16(battery)}, sizeof(battery));
		tx_data_len += sizeof(battery);
		sx1278_send(&tx_data, tx_data_len, 1000);

		rp2040_led_blink(3, 50, true);

		// rx_data_len = 0;
		// sx1278_recv(&rx_data, &rx_data_len, 1000);

		// if (rx_data_len != 0) {
		// 	rp2040_led_blink(4, 50, true);
		// }

		sx1278_standby();
		sx1278_sleep();

		// sleep_ms(10 * 1000);

		pcf8563_alarm_schedule(1);
		printf("entering dormant sleep\n");

		sleep_run_from_xosc();
		sleep_goto_dormant_until_pin(pcf8563_int_pin, true, false);

		rp2040_led_init();
		pcf8563_init();
		si7021_init();
		ads1115_init();
		sx1278_init();

		pcf8563_alarm_clear();
		printf("woke up from dormant sleep\n");
	}
}
