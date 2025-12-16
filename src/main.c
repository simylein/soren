#include "app.h"
#include "buffer.h"
#include "config.h"
#include "endian.h"
#include "logger.h"
#include "math.h"
#include "pcf8563.h"
#include "rp2040.h"
#include "si7021.h"
#include "sleep.h"
#include "sx1278.h"
#include <math.h>
#include <pico/sleep.h>
#include <pico/stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
	if (deep_sleep == true) {
		rp2040_clocks(rp2040_clock_speed);
	}

	if (deep_sleep == false) {
		rp2040_stdio_init();
	}

	config_t config;
	config_read(&config);

	srand((config.id[0] << 8) | config.id[1]);

	info("starting soren sensor firmware\n");

	pcf8563_init();
	si7021_init();
	sx1278_init();
	rp2040_adc_init();
	if (config.led_debug == true) {
		rp2040_led_init();
	}

	sx1278_reset();

	if (sx1278_sleep(timeout) == -1) {
		error("sx1278 failed to enter sleep\n");
	}

	configure(&config);

	if (sx1278_sleep(timeout) == -1) {
		error("sx1278 failed to enter sleep\n");
	}

	if (config.led_debug == true) {
		rp2040_led_blink(8);
	}

	datetime_t datetime;
	if (pcf8563_datetime(&datetime) == -1) {
		error("pcf8563 failed to read datetime\n");
		goto sleep;
	}

	time_t captured_at;
	if (datetime_to_time(&datetime, &captured_at) == false) {
		error("failed to convert datetime\n");
		goto sleep;
	}

	uplink_t uplink = {.kind = 0x04, .data_len = 0, .captured_at = captured_at};
	memcpy(&uplink.data[uplink.data_len], config.firmware, sizeof(config.firmware));
	uplink.data_len += sizeof(config.firmware);
	memcpy(&uplink.data[uplink.data_len], config.hardware, sizeof(config.hardware));
	uplink.data_len += sizeof(config.hardware);

	if (transceive(&config, &uplink) == -1) {
		buffer_push(&uplink);
		info("buffered uplink at size %hu\n", buffer.size);
	}

	bool acknowledged = false;
	uint16_t jitter = 0;

	uint16_t next_reading = 0;
	uint16_t next_metric = 0;
	uint16_t next_buffer = 3600;

	while (true) {
		if (config.led_debug == true) {
			rp2040_led_blink(1);
		}

		datetime_t datetime;
		if (pcf8563_datetime(&datetime) == -1) {
			error("pcf8563 failed to read datetime\n");
			goto sleep;
		}

		time_t captured_at;
		if (datetime_to_time(&datetime, &captured_at) == false) {
			error("failed to convert datetime\n");
			goto sleep;
		}

		info("datetime %02hhu:%02hhu:%02hhu captured at %lld\n", datetime.hour, datetime.min, datetime.sec, captured_at);

		bool do_reading = next_reading == 0 && config.reading_enable == true;
		bool do_metric = next_metric == 0 && config.metric_enable == true;
		bool do_buffer = next_buffer == 0 && config.buffer_enable == true;

		uplink_t uplink = {.data_len = 0, .captured_at = captured_at};

		if (do_reading == true && do_metric == true) {
			uplink.kind = 0x03;
		} else if (do_reading == true) {
			uplink.kind = 0x01;
		} else if (do_metric == true) {
			uplink.kind = 0x02;
		} else {
			uplink.kind = 0x00;
		}

		if (do_reading == true) {
			uint16_t temperature;
			uint16_t humidity;

			if (si7021_temperature(&temperature, timeout) == -1) {
				error("si7021 failed to read temperature\n");
				goto sleep;
			};

			if (si7021_humidity(&humidity, timeout) == -1) {
				error("si7021 failed to read humidity\n");
				goto sleep;
			};

			info("temperature %.2f humidity %.2f\n", si7021_temperature_human(temperature), si7021_humidity_human(humidity));

			memcpy(&uplink.data[uplink.data_len], (uint16_t[]){hton16(temperature)}, sizeof(temperature));
			uplink.data_len += sizeof(temperature);
			memcpy(&uplink.data[uplink.data_len], (uint16_t[]){hton16(humidity)}, sizeof(humidity));
			uplink.data_len += sizeof(humidity);
		}

		if (do_metric == true) {
			uint16_t photovoltaic;
			uint16_t battery;

			rp2040_photovoltaic(&photovoltaic, 100);
			rp2040_battery(&battery, 100);

			info("photovoltaic %.3f battery %.3f\n", rp2040_photovoltaic_human(photovoltaic), rp2040_battery_human(battery));

			uint8_t packed[3];
			packed[0] = (uint8_t)(photovoltaic >> 4);
			packed[1] = (uint8_t)((photovoltaic & 0x0f) << 4) | (uint8_t)(battery >> 8);
			packed[2] = (uint8_t)(battery & 0xff);
			memcpy(&uplink.data[uplink.data_len], packed, sizeof(packed));
			uplink.data_len += sizeof(packed);
		}

		if (config.led_debug == true) {
			rp2040_led_blink(2);
		}

		if (sx1278_standby(timeout) == -1) {
			error("sx1278 failed to enter standby\n");
		}

		if (uplink.kind != 0x00) {
			if (transceive(&config, &uplink) == -1) {
				buffer_push(&uplink);
				info("buffered uplink at size %hu\n", buffer.size);
				next_buffer = config.buffer_interval;
				acknowledged = false;
				jitter = rand() % (int)pow(2, config.spreading_factor - 4);
				goto sleep;
			}
			acknowledged = true;
			jitter = 0;
		}

		if (do_buffer == true && buffer.size == 0) {
			sleep_ms(5);
			info("buffer offloaded size %hu\n", buffer.size);

			uplink.kind = 0x80;
			uplink.data_len = 5;
			memset(uplink.data, 0x00, uplink.data_len);

			if (transceive(&config, &uplink) == -1) {
				acknowledged = false;
				jitter = rand() % (int)pow(2, config.spreading_factor - 4);
				goto sleep;
			}

			next_buffer = 3600;
		}

		if (do_buffer == true && buffer.size > 0) {
			sleep_ms(5);
			info("offloading buffer at size %hu\n", buffer.size);

			uplink_t uplink;
			buffer_peek(&uplink);

			datetime_t datetime;
			if (pcf8563_datetime(&datetime) == -1) {
				error("pcf8563 failed to read datetime\n");
				goto sleep;
			}

			time_t captured_at;
			if (datetime_to_time(&datetime, &captured_at) == false) {
				error("failed to convert datetime\n");
				goto sleep;
			}

			info("datetime %02hhu:%02hhu:%02hhu captured at %lld\n", datetime.hour, datetime.min, datetime.sec, captured_at);

			time_t delay = captured_at - uplink.captured_at;

			uplink.kind |= 0x80;

			uplink.data[uplink.data_len] = (delay >> 16) & 0xff;
			uplink.data_len += 1;
			uplink.data[uplink.data_len] = (delay >> 8) & 0xff;
			uplink.data_len += 1;
			uplink.data[uplink.data_len] = delay & 0xff;
			uplink.data_len += 1;
			memcpy(&uplink.data[uplink.data_len], (uint16_t[]){hton16(buffer.size)}, sizeof(buffer.size));
			uplink.data_len += sizeof(buffer.size);

			if (transceive(&config, &uplink) == -1) {
				acknowledged = false;
				jitter = rand() % (int)pow(2, config.spreading_factor - 4);
				goto sleep;
			}

			buffer_pop();
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
		if (next_buffer == 0) {
			next_buffer = config.buffer_interval;
		}

		interval = min16(next_reading, next_metric);
		if (acknowledged == true) {
			interval = min16(interval, next_buffer);
		}

		trace("next reading in %hu seconds\n", next_reading);
		trace("next metric in %hu seconds\n", next_metric);
		trace("next buffer in %hu seconds\n", next_buffer);
		debug("sleeping for %hu seconds and %hu milliseconds\n", interval, jitter);

		uint32_t duration = interval * 1000 + jitter;
		sleep(duration);

		debug("woke up from sleep\n");
		next_reading = sub16(next_reading, interval);
		next_metric = sub16(next_metric, interval);
		next_buffer = sub16(next_buffer, interval);
	}
}
