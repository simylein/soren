#include "config.h"
#include "endian.h"
#include "logger.h"
#include "rp2040.h"
#include "si7021.h"
#include "sx1278.h"
#include <pico/stdlib.h>
#include <string.h>

int main(void) {
	rp2040_stdio_init();

	info("starting soren sensor firmware\n");

	si7021_init();
	sx1278_init();

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

	while (true) {
		uint16_t temperature;
		if (si7021_temperature(&temperature, timeout) == -1) {
			error("si7021 failed to read temperature\n");
			goto sleep;
		};

		uint16_t humidity;
		if (si7021_humidity(&humidity, timeout) == -1) {
			error("si7021 failed to read humidity\n");
			goto sleep;
		};

		info("temperature %.2f humidity %.2f\n", si7021_temperature_human(temperature), si7021_humidity_human(humidity));

		if (sx1278_standby(timeout) == -1) {
			error("sx1278 failed to enter standby\n");
		}

		uint8_t tx_data[256];
		uint8_t tx_data_len = 0;

		memcpy(&tx_data[tx_data_len], config.id, sizeof(config.id));
		tx_data_len += sizeof(config.id);
		tx_data[tx_data_len] = 0x01;
		tx_data_len += 1;
		memcpy(&tx_data[tx_data_len], (uint16_t[]){hton16(temperature)}, sizeof(temperature));
		tx_data_len += sizeof(temperature);
		memcpy(&tx_data[tx_data_len], (uint16_t[]){hton16(humidity)}, sizeof(humidity));
		tx_data_len += sizeof(humidity);

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

		debug("sleeping for %d seconds\n", config.interval);
		sleep_ms(config.interval * 1000);
	}
}
