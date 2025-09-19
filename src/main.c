#include "app.h"
#include "buffer.h"
#include "config.h"
#include "ds3231.h"
#include "endian.h"
#include "logger.h"
#include "rp2040.h"
#include "si7021.h"
#include "sx1278.h"
#include <hardware/rtc.h>
#include <pico/sleep.h>
#include <pico/stdlib.h>
#include <pico/types.h>
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
	rp2040_rtc_init();

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
		time_t captured_at = time(NULL);

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

		uplink_t uplink = {.data_len = 0, captured_at = captured_at};

		if (reading == true && metric == true) {
			uplink.kind = 0x03;
		} else if (reading == true) {
			uplink.kind = 0x01;
		} else if (metric == true) {
			uplink.kind = 0x02;
		} else {
			uplink.kind = 0x00;
		}

		if (reading == true) {
			memcpy(&uplink.data[uplink.data_len], (uint16_t[]){hton16(temperature)}, sizeof(temperature));
			uplink.data_len += sizeof(temperature);
			memcpy(&uplink.data[uplink.data_len], (uint16_t[]){hton16(humidity)}, sizeof(humidity));
			uplink.data_len += sizeof(humidity);
		}
		if (metric == true) {
			uint8_t packed[3];
			packed[0] = (uint8_t)(photovoltaic >> 4);
			packed[1] = (uint8_t)((photovoltaic & 0x0f) << 4) | (uint8_t)(battery >> 8);
			packed[2] = (uint8_t)(battery & 0xff);
			memcpy(&uplink.data[uplink.data_len], packed, sizeof(packed));
			uplink.data_len += sizeof(packed);
		}

		if (transceive(&config, &uplink) == -1) {
			if (uplink.kind != 0x00) {
				buffer_push(&uplink);
				info("buffered uplink at size %hu\n", buffer.size);
			}
			goto sleep;
		}

		if (buffer.size > 0) {
			sleep_ms(50);
			info("offloading buffer at size %hu\n", buffer.size);

			uplink_t uplink;
			buffer_peek(&uplink);

			time_t now = time(NULL);
			time_t delta = now - uplink.captured_at;

			uplink.kind |= 0x80;

			uplink.data[uplink.data_len] = (delta >> 16) & 0xff;
			uplink.data_len += 1;
			uplink.data[uplink.data_len] = (delta >> 8) & 0xff;
			uplink.data_len += 1;
			uplink.data[uplink.data_len] = delta & 0xff;
			uplink.data_len += 1;
			memcpy(&uplink.data[uplink.data_len], (uint16_t[]){hton16(buffer.size)}, sizeof(buffer.size));
			uplink.data_len += sizeof(buffer.size);

			if (transceive(&config, &uplink) == -1) {
				goto sleep;
			}

			buffer_pop();

			if (buffer.size == 0) {
				sleep_ms(50);
				uplink.kind = 0x80;
				uplink.data_len = 5;
				memset(uplink.data, 0x00, uplink.data_len);
				if (transceive(&config, &uplink) == -1) {
					goto sleep;
				}
			}
		}

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
			rp2040_rtc_init();

			if (ds3231_alarm_clear() == -1) {
				error("ds3231 failed to clear alarm\n");
			}

			datetime_t datetime;
			if (ds3231_datetime(&datetime) == -1) {
				error("ds3231 failed to read datetime\n");
			}

			rtc_set_datetime(&datetime);
		}

		debug("woke up from sleep\n");
		next_reading -= interval;
		next_metric -= interval;
	}
}
