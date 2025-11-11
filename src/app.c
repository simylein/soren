#include "app.h"
#include "config.h"
#include "endian.h"
#include "logger.h"
#include "rp2040.h"
#include "sx1278.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

int transceive(config_t *config, uplink_t *uplink) {
	uint8_t tx_data[256];
	uint8_t tx_data_len = 0;

	memcpy(&tx_data[tx_data_len], config->id, sizeof(config->id));
	tx_data_len += sizeof(config->id);
	tx_data[tx_data_len] = ((config->tx_power - 2) << 4) & 0xf0;
	tx_data_len += sizeof(config->tx_power);
	tx_data[tx_data_len] = uplink->kind;
	tx_data_len += sizeof(uplink->kind);
	memcpy(&tx_data[tx_data_len], uplink->data, uplink->data_len);
	tx_data_len += uplink->data_len;

	if (sx1278_transmit(&tx_data, tx_data_len, 2048 + (rand() % 512)) == -1) {
		error("sx1278 failed to transmit packet\n");
		return -1;
	}

	tx("id %02x%02x kind %02x bytes %hhu sf %hhu power %hhu\n", tx_data[0], tx_data[1], tx_data[3], tx_data_len,
		 config->spreading_factor, ((tx_data[2] >> 4) & 0x0f) + 2);

	if (led_debug == true) {
		sx1278_rx(timeout);
		rp2040_led_blink(3);
	}

	uint8_t rx_data[256];
	uint8_t rx_data_len = 0;
	if (sx1278_receive(&rx_data, &rx_data_len, 2048 + (rand() % 512)) == -1) {
		error("sx1278 failed to receive packet\n");
		return -1;
	}

	if (rx_data_len < 4) {
		debug("sx1278 received packet without headers\n");
		return -1;
	}

	if (memcmp(rx_data, config->id, sizeof(config->id)) != 0) {
		warn("downlink id %02x%02x does not match device id %02x%02x\n", rx_data[0], rx_data[1], config->id[0], config->id[1]);
		return -1;
	}

	int16_t rssi;
	if (sx1278_rssi(&rssi) == -1) {
		error("sx1278 failed to read packet rssi\n");
		return -1;
	}

	int8_t snr;
	if (sx1278_snr(&snr) == -1) {
		error("sx1278 failed to read packet snr\n");
		return -1;
	}

	rx("id %02x%02x kind %02x bytes %hhu rssi %hd snr %.2f sf %hhu power %hhu\n", rx_data[0], rx_data[1], rx_data[3], rx_data_len,
		 rssi, snr / 4.0f, config->spreading_factor, ((rx_data[2] >> 4) & 0x0f) + 2);

	if (led_debug == true) {
		rp2040_led_blink(4);
	}

	if (rx_data[3] == 0x05 && rx_data_len == 11) {
		bool reading_enable = (bool)(rx_data[4] & 0x80);
		bool metric_enable = (bool)(rx_data[4] & 0x40);
		bool buffer_enable = (bool)(rx_data[4] & 0x20);
		uint16_t reading_interval = (uint16_t)((rx_data[5] << 8) | rx_data[6]);
		uint16_t metric_interval = (uint16_t)((rx_data[7] << 8) | rx_data[8]);
		uint16_t buffer_interval = (uint16_t)((rx_data[9] << 8) | rx_data[10]);

		if (reading_interval < 8 || reading_interval > 4096) {
			warn("invalid reading interval %hu\n", reading_interval);
			return 0;
		}

		if (metric_interval < 8 || metric_interval > 4096) {
			warn("invalid metric interval %hu\n", metric_interval);
			return 0;
		}

		if (buffer_interval < 8 || buffer_interval > 4096) {
			warn("invalid buffer interval %hu\n", buffer_interval);
			return 0;
		}

		config->reading_enable = reading_enable;
		config->metric_enable = metric_enable;
		config->buffer_enable = buffer_enable;
		config->reading_interval = reading_interval;
		config->metric_interval = metric_interval;
		config->buffer_interval = buffer_interval;
		config_write(config);
	}

	return 0;
}
