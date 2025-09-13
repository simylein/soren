#include "config.h"
#include "ds3231.h"
#include "endian.h"
#include "logger.h"
#include "rp2040.h"
#include "si7021.h"
#include "sx1278.h"
#include <pico/sleep.h>
#include <pico/stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

const bool prod = true;

int main(void) {
	if (!prod) {
		rp2040_stdio_init();
	}

	info("starting soren sensor firmware\n");

	ds3231_init();
	si7021_init();
	sx1278_init();
	rp2040_adc_init();

	if (sx1278_sleep(timeout) == -1) {
		error("sx1278 failed to enter sleep\n");
	}

	config_t config;
	config_read(&config);

	if (sx1278_standby(timeout) == -1) {
		error("sx1278 failed to enter standby\n");
	}

	if (sx1278_frequency(config.frequency) == -1) {
		error("sx1278 failed to configure frequency\n");
	}

	if (sx1278_tx_power(config.tx_power) == -1) {
		error("sx1278 failed to configure tx power\n");
	}

	if (sx1278_coding_rate(config.coding_rate) == -1) {
		error("sx1278 failed to configure coding rate\n");
	}

	if (sx1278_bandwidth(config.bandwidth) == -1) {
		error("sx1278 failed to configure bandwidth\n");
	}

	if (sx1278_spreading_factor(config.spreading_factor) == -1) {
		error("sx1278 failed to configure spreading factor\n");
	}

	if (sx1278_checksum(config.checksum) == -1) {
		error("sx1278 failed to configure checksum\n");
	}

	if (sx1278_sync_word(config.sync_word) == -1) {
		error("sx1278 failed to configure sync word\n");
	}

	if (sx1278_sleep(timeout) == -1) {
		error("sx1278 failed to enter sleep\n");
	}

	uint16_t next_reading = 0;
	uint16_t next_metric = 0;
	while (true) {
		bool reading = next_reading == 0 && config.reading_enable == true;
		bool metric = next_metric == 0 && config.metric_enable == true;

		uint16_t temperature;
		uint16_t humidity;
		if (reading == true) {
			if (si7021_temperature(&temperature, timeout) == -1) {
				error("si7021 failed to read temperature\n");
				goto sleep;
			};

			if (si7021_humidity(&humidity, timeout) == -1) {
				error("si7021 failed to read humidity\n");
				goto sleep;
			};

			info("temperature %.2f humidity %.2f\n", si7021_temperature_human(temperature), si7021_humidity_human(humidity));
		}

		uint16_t photovoltaic;
		uint16_t battery;
		if (metric == true) {
			rp2040_photovoltaic(&photovoltaic, 5);
			rp2040_battery(&battery, 5);

			info("photovoltaic %.3f battery %.3f\n", rp2040_photovoltaic_human(photovoltaic), rp2040_battery_human(battery));
		}

		if (sx1278_standby(timeout) == -1) {
			error("sx1278 failed to enter standby\n");
		}

		uint8_t tx_data[256];
		uint8_t tx_data_len = 0;

		memcpy(&tx_data[tx_data_len], config.id, sizeof(config.id));
		tx_data_len += sizeof(config.id);
		if (reading == true && metric == true) {
			tx_data[tx_data_len] = 0x03;
		} else if (reading == true) {
			tx_data[tx_data_len] = 0x01;
		} else if (metric == true) {
			tx_data[tx_data_len] = 0x02;
		} else {
			tx_data[tx_data_len] = 0x00;
		}
		tx_data_len += 1;
		if (reading == true) {
			memcpy(&tx_data[tx_data_len], (uint16_t[]){hton16(temperature)}, sizeof(temperature));
			tx_data_len += sizeof(temperature);
			memcpy(&tx_data[tx_data_len], (uint16_t[]){hton16(humidity)}, sizeof(humidity));
			tx_data_len += sizeof(humidity);
		}
		if (metric == true) {
			uint8_t packed[3];
			packed[0] = (uint8_t)(photovoltaic >> 4);
			packed[1] = (uint8_t)((photovoltaic & 0x0f) << 4) | (uint8_t)(battery >> 8);
			packed[2] = (uint8_t)(battery & 0xff);
			memcpy(&tx_data[tx_data_len], packed, sizeof(packed));
			tx_data_len += sizeof(packed);
		}

		if (sx1278_transmit(&tx_data, tx_data_len, 2048)) {
			error("sx1278 failed to transmit packet\n");
			goto sleep;
		}

		tx("id %02x%02x kind %02x bytes %hhu power %hhu sf %hhu\n", tx_data[0], tx_data[1], tx_data[2], tx_data_len,
			 config.tx_power, config.spreading_factor);

		uint8_t rx_data[256];
		uint8_t rx_data_len = 0;
		if (sx1278_receive(&rx_data, &rx_data_len, 2048) == -1) {
			error("sx1278 failed to receive packet\n");
			goto sleep;
		}

		if (rx_data_len < 3) {
			debug("sx1278 received packet without headers\n");
			goto sleep;
		}

		int16_t rssi;
		if (sx1278_rssi(&rssi) == -1) {
			error("sx1278 failed to read packet rssi\n");
			goto sleep;
		}

		int8_t snr;
		if (sx1278_snr(&snr) == -1) {
			error("sx1278 failed to read packet snr\n");
			goto sleep;
		}

		rx("id %02x%02x kind %02x bytes %hhu rssi %hd snr %.2f sf %hhu\n", rx_data[0], rx_data[1], rx_data[2], rx_data_len, rssi,
			 snr / 4.0f, config.spreading_factor);

		if (sx1278_standby(timeout) == -1) {
			error("sx1278 failed to enter standby\n");
		}

	sleep:
		if (sx1278_sleep(timeout) == -1) {
			error("sx1278 failed to enter sleep\n");
		}

		uint16_t interval;
		if (next_reading == 0) {
			next_reading = config.reading_interval;
		}
		if (next_metric == 0) {
			next_metric = config.metric_interval;
		}
		if (next_reading < next_metric) {
			interval = next_reading;
		} else {
			interval = next_metric;
		}
		trace("next reading in %hu seconds\n", next_reading);
		trace("next metric in %hu seconds\n", next_metric);
		debug("sleeping for %hu seconds\n", interval);

		if (!prod) {
			sleep_ms(interval * 1000);
		} else {
			if (ds3231_alarm(interval) == -1) {
				error("ds3231 failed to write alarm\n");
			}

			sleep_run_from_xosc();
			sleep_goto_dormant_until_pin(ds3231_pin_int, true, false);
			sleep_power_up();

			ds3231_init();
			si7021_init();
			sx1278_init();
			rp2040_adc_init();
		}

		debug("woke up from sleep\n");
		next_reading -= interval;
		next_metric -= interval;
	}
}
