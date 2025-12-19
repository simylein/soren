#include "app.h"
#include "buffer.h"
#include "config.h"
#include "endian.h"
#include "logger.h"
#include "pcf8563.h"
#include "rp2040.h"
#include "sleep.h"
#include "sx1278.h"
#include <math.h>
#include <pico/sleep.h>
#include <pico/stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

int configure(config_t *config) {
	if (sx1278_standby(timeout) == -1) {
		error("sx1278 failed to enter standby\n");
		return -1;
	}

	if (sx1278_frequency(config->frequency) == -1) {
		error("sx1278 failed to configure frequency\n");
		return -1;
	}

	if (sx1278_tx_power(config->tx_power) == -1) {
		error("sx1278 failed to configure tx power\n");
		return -1;
	}

	if (sx1278_preamble_length(config->preamble_length) == -1) {
		error("sx1278 failed to configure preamble length\n");
		return -1;
	}

	if (sx1278_coding_rate(config->coding_rate) == -1) {
		error("sx1278 failed to configure coding rate\n");
		return -1;
	}

	if (sx1278_bandwidth(config->bandwidth) == -1) {
		error("sx1278 failed to configure bandwidth\n");
		return -1;
	}

	if (sx1278_spreading_factor(config->spreading_factor) == -1) {
		error("sx1278 failed to configure spreading factor\n");
		return -1;
	}

	if (sx1278_checksum(config->checksum) == -1) {
		error("sx1278 failed to configure checksum\n");
		return -1;
	}

	if (sx1278_sync_word(config->sync_word) == -1) {
		error("sx1278 failed to configure sync word\n");
		return -1;
	}

	return 0;
}

int transceive(config_t *config, uplink_t *uplink) {
	uint8_t tx_data[256];
	uint8_t tx_data_len = 0;

	memcpy(&tx_data[tx_data_len], config->id, sizeof(config->id));
	tx_data_len += sizeof(config->id);
	tx_data[tx_data_len] = ((config->tx_power - 2) << 4) & 0xf0 | ((config->preamble_length - 6) & 0x0f);
	tx_data_len += sizeof(tx_data[tx_data_len]);
	tx_data[tx_data_len] = uplink->kind;
	tx_data_len += sizeof(uplink->kind);
	memcpy(&tx_data[tx_data_len], uplink->data, uplink->data_len);
	tx_data_len += uplink->data_len;

	if (sx1278_transmit(&tx_data, tx_data_len, (uint32_t)pow(2, config->spreading_factor - 4) * 16 + 16) == -1) {
		error("sx1278 failed to transmit packet\n");
		return -1;
	}

	tx("id %02x%02x kind %02x bytes %hhu sf %hhu power %hhu\n", tx_data[0], tx_data[1], tx_data[3], tx_data_len,
		 config->spreading_factor, ((tx_data[2] >> 4) & 0x0f) + 2);

	if (config->led_debug == true) {
		sx1278_rx(timeout);
		rp2040_led_blink(3);
	}

	uint8_t rx_data[256];
	uint8_t rx_data_len = 0;
	if (sx1278_receive(&rx_data, &rx_data_len, (uint32_t)pow(2, config->spreading_factor - 4) * 16 + 16) == -1) {
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

	if (config->led_debug == true) {
		rp2040_led_blink(4);
	}

	if (rx_data[3] == 0x04 && rx_data_len == 4) {
		sleep(20);
		transceive_version(config);
	}

	if (rx_data[3] == 0x05 && rx_data_len == 4) {
		sleep(20);
		transceive_config(config);
	}

	if (rx_data[3] == 0x05 && rx_data_len == 11) {
		bool led_debug = (bool)(rx_data[4] & 0x80);
		bool reading_enable = (bool)(rx_data[4] & 0x40);
		bool metric_enable = (bool)(rx_data[4] & 0x20);
		bool buffer_enable = (bool)(rx_data[4] & 0x10);
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

		config->led_debug = led_debug;
		config->reading_enable = reading_enable;
		config->metric_enable = metric_enable;
		config->buffer_enable = buffer_enable;
		config->reading_interval = reading_interval;
		config->metric_interval = metric_interval;
		config->buffer_interval = buffer_interval;
		config_write(config);
		config_read(config);
	}

	if (rx_data[3] == 0x06 && rx_data_len == 4) {
		sleep(20);
		transceive_radio(config);
	}

	if (rx_data[3] == 0x06 && rx_data_len == 17) {
		uint32_t frequency = (uint32_t)((rx_data[4] << 24) | (rx_data[5] << 16) | (rx_data[6] << 8) | rx_data[7]);
		uint32_t bandwidth = (uint32_t)((rx_data[8] << 16) | (rx_data[9] << 8) | rx_data[10]);
		uint8_t coding_rate = rx_data[11];
		uint8_t spreading_factor = rx_data[12];
		uint8_t preamble_length = rx_data[13];
		uint8_t tx_power = rx_data[14];
		uint8_t sync_word = rx_data[15];
		bool checksum = (bool)rx_data[16];

		if (frequency < 400 * 1000 * 1000 || frequency > 500 * 1000 * 1000) {
			warn("invalid frequency %u\n", frequency);
			return 0;
		}

		if (bandwidth < 7800 || bandwidth > 500 * 1000) {
			warn("invalid bandwidth %u\n", bandwidth);
			return 0;
		}

		if (coding_rate < 5 || coding_rate > 8) {
			warn("invalid coding rate %hhu\n", coding_rate);
			return -1;
		}

		if (spreading_factor < 7 || spreading_factor > 12) {
			warn("invalid spreading factor %hhu\n", spreading_factor);
			return -1;
		}

		if (preamble_length < 6 || preamble_length > 21) {
			warn("invalid preamble length %hhu\n", preamble_length);
			return -1;
		}

		if (tx_power < 2 || tx_power > 17) {
			warn("invalid tx power %hhu\n", tx_power);
			return -1;
		}

		config->frequency = frequency;
		config->bandwidth = bandwidth;
		config->coding_rate = coding_rate;
		config->spreading_factor = spreading_factor;
		config->preamble_length = preamble_length;
		config->tx_power = tx_power;
		config->sync_word = sync_word;
		config->checksum = checksum;
		config_write(config);
		config_read(config);
		configure(config);
	}

	return 0;
}

int transceive_version(config_t *config) {
	datetime_t datetime;
	if (pcf8563_datetime(&datetime) == -1) {
		error("pcf8563 failed to read datetime\n");
		return -1;
	}

	time_t captured_at;
	if (datetime_to_time(&datetime, &captured_at) == false) {
		error("failed to convert datetime\n");
		return -1;
	}

	uplink_t uplink = {.kind = 0x04, .data_len = 0, .captured_at = captured_at};
	memcpy(&uplink.data[uplink.data_len], config->firmware, sizeof(config->firmware));
	uplink.data_len += sizeof(config->firmware);
	memcpy(&uplink.data[uplink.data_len], config->hardware, sizeof(config->hardware));
	uplink.data_len += sizeof(config->hardware);

	if (transceive(config, &uplink) == -1) {
		buffer_push(&uplink);
		info("buffered uplink at size %hu\n", buffer.size);
		return -1;
	}

	return 0;
}

int transceive_config(config_t *config) {
	datetime_t datetime;
	if (pcf8563_datetime(&datetime) == -1) {
		error("pcf8563 failed to read datetime\n");
		return -1;
	}

	time_t captured_at;
	if (datetime_to_time(&datetime, &captured_at) == false) {
		error("failed to convert datetime\n");
		return -1;
	}

	uplink_t uplink = {.kind = 0x05, .data_len = 0, .captured_at = captured_at};
	uplink.data[uplink.data_len] = 0x00;
	uplink.data[uplink.data_len] |= config->led_debug << 7;
	uplink.data[uplink.data_len] |= config->reading_enable << 6;
	uplink.data[uplink.data_len] |= config->metric_enable << 5;
	uplink.data[uplink.data_len] |= config->buffer_enable << 4;
	uplink.data_len += sizeof(uint8_t);
	memcpy(&uplink.data[uplink.data_len], (uint16_t[]){hton16(config->reading_interval)}, sizeof(config->reading_interval));
	uplink.data_len += sizeof(config->reading_interval);
	memcpy(&uplink.data[uplink.data_len], (uint16_t[]){hton16(config->metric_interval)}, sizeof(config->metric_interval));
	uplink.data_len += sizeof(config->metric_interval);
	memcpy(&uplink.data[uplink.data_len], (uint16_t[]){hton16(config->buffer_interval)}, sizeof(config->buffer_interval));
	uplink.data_len += sizeof(config->buffer_interval);

	if (transceive(config, &uplink) == -1) {
		buffer_push(&uplink);
		info("buffered uplink at size %hu\n", buffer.size);
		return -1;
	}

	return 0;
}

int transceive_radio(config_t *config) {
	datetime_t datetime;
	if (pcf8563_datetime(&datetime) == -1) {
		error("pcf8563 failed to read datetime\n");
		return -1;
	}

	time_t captured_at;
	if (datetime_to_time(&datetime, &captured_at) == false) {
		error("failed to convert datetime\n");
		return -1;
	}

	uplink_t uplink = {.kind = 0x06, .data_len = 0, .captured_at = captured_at};
	memcpy(&uplink.data[uplink.data_len], (uint32_t[]){hton32(config->frequency)}, sizeof(config->frequency));
	uplink.data_len += sizeof(config->frequency);
	uplink.data[uplink.data_len] = (config->bandwidth >> 16) & 0xff;
	uplink.data_len += sizeof(uint8_t);
	uplink.data[uplink.data_len] = (config->bandwidth >> 8) & 0xff;
	uplink.data_len += sizeof(uint8_t);
	uplink.data[uplink.data_len] = config->bandwidth & 0xff;
	uplink.data_len += sizeof(uint8_t);
	uplink.data[uplink.data_len] = config->coding_rate;
	uplink.data_len += sizeof(config->coding_rate);
	uplink.data[uplink.data_len] = config->spreading_factor;
	uplink.data_len += sizeof(config->spreading_factor);
	uplink.data[uplink.data_len] = config->preamble_length;
	uplink.data_len += sizeof(config->preamble_length);
	uplink.data[uplink.data_len] = config->tx_power;
	uplink.data_len += sizeof(config->tx_power);
	uplink.data[uplink.data_len] = config->sync_word;
	uplink.data_len += sizeof(config->sync_word);
	uplink.data[uplink.data_len] = config->checksum;
	uplink.data_len += sizeof(config->checksum);

	if (transceive(config, &uplink) == -1) {
		buffer_push(&uplink);
		info("buffered uplink at size %hu\n", buffer.size);
		return -1;
	}

	return 0;
}
