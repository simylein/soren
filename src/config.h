#pragma once

#include <hardware/i2c.h>
#include <hardware/spi.h>
#include <pico/stdlib.h>

typedef struct config_t {
	uint8_t id[2];
	uint8_t firmware[2];
	uint8_t hardware[2];
	bool led_debug;
	bool reading_enable;
	bool metric_enable;
	bool buffer_enable;
	uint16_t reading_interval;
	uint16_t metric_interval;
	uint16_t buffer_interval;
	uint32_t frequency;
	uint32_t bandwidth;
	uint8_t coding_rate;
	uint8_t spreading_factor;
	uint8_t tx_power;
	uint8_t sync_word;
	bool checksum;
} config_t;

extern const bool deep_sleep;

extern const char *name;

extern const uint8_t log_level;
extern const bool log_receives;
extern const bool log_transmits;

extern const uint32_t timeout;

extern const uint rp2040_clock_speed;
extern const uint rp2040_pin_led;
extern const uint rp2040_pin_photovoltaic;
extern const uint rp2040_adc_photovoltaic;
extern const uint rp2040_en_photovoltaic;
extern const uint rp2040_pin_battery;
extern const uint rp2040_adc_battery;
extern const uint rp2040_en_battery;

extern i2c_inst_t *const ds3231_i2c_inst;
extern const uint ds3231_i2c_speed;
extern const uint ds3231_pin_sda;
extern const uint ds3231_pin_scl;
extern const uint ds3231_pin_int;
extern const uint8_t ds3231_addr;

extern i2c_inst_t *const pcf8563_i2c_inst;
extern const uint pcf8563_i2c_speed;
extern const uint pcf8563_pin_sda;
extern const uint pcf8563_pin_scl;
extern const uint pcf8563_pin_int;
extern const uint8_t pcf8563_addr;

extern i2c_inst_t *const si7021_i2c_inst;
extern const uint si7021_i2c_speed;
extern const uint si7021_pin_sda;
extern const uint si7021_pin_scl;
extern const uint8_t si7021_addr;

extern spi_inst_t *const sx1278_spi_inst;
extern const uint sx1278_spi_speed;
extern const uint sx1278_pin_miso;
extern const uint sx1278_pin_nss;
extern const uint sx1278_pin_sck;
extern const uint sx1278_pin_mosi;
extern const uint sx1278_pin_reset;

void config_write(config_t *config);
void config_read(config_t *config);
