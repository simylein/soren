#include "config.h"
#include "endian.h"
#include "format.h"
#include "logger.h"
#include <hardware/flash.h>
#include <hardware/i2c.h>
#include <hardware/spi.h>
#include <hardware/sync.h>
#include <pico/stdlib.h>
#include <string.h>

config_t config;

const bool deep_sleep = true;

const char *name = "soren";

const uint8_t log_level = 4;
const bool log_receives = true;
const bool log_transmits = true;

const uint32_t timeout = 256;

const uint rp2040_clock_speed = 16 * 1000 * 1000;
const uint rp2040_pin_led = 25;
const uint rp2040_pin_photovoltaic = 26;
const uint rp2040_adc_photovoltaic = 0;
const uint rp2040_en_photovoltaic = 10;
const uint rp2040_pin_battery = 27;
const uint rp2040_adc_battery = 1;
const uint rp2040_en_battery = 11;

i2c_inst_t *const ds3231_i2c_inst = i2c0;
const uint ds3231_i2c_speed = 1 * 100 * 1000;
const uint ds3231_pin_sda = 0;
const uint ds3231_pin_scl = 1;
const uint ds3231_pin_int = 2;
const uint8_t ds3231_addr = 0x68;

i2c_inst_t *const pcf8563_i2c_inst = i2c0;
const uint pcf8563_i2c_speed = 1 * 100 * 1000;
const uint pcf8563_pin_sda = 0;
const uint pcf8563_pin_scl = 1;
const uint pcf8563_pin_int = 2;
const uint8_t pcf8563_addr = 0x51;

i2c_inst_t *const si7021_i2c_inst = i2c1;
const uint si7021_i2c_speed = 1 * 100 * 1000;
const uint si7021_pin_sda = 6;
const uint si7021_pin_scl = 7;
const uint8_t si7021_addr = 0x40;

spi_inst_t *const sx1278_spi_inst = spi0;
const uint sx1278_spi_speed = 1 * 1000 * 1000;
const uint sx1278_pin_miso = 16;
const uint sx1278_pin_nss = 17;
const uint sx1278_pin_sck = 18;
const uint sx1278_pin_mosi = 19;
const uint sx1278_pin_reset = 20;

void config_write(config_t *config) {
	trace("config write id 0x%02x%02x\n", config->id[0], config->id[1]);
	trace("config write firmware 0x%02x%02x\n", config->firmware[0], config->firmware[1]);
	trace("config write hardware 0x%02x%02x\n", config->hardware[0], config->hardware[1]);
	trace("config write led debug %s\n", human_bool(config->led_debug));
	trace("config write reading enable %s\n", human_bool(config->reading_enable));
	trace("config write metric enable %s\n", human_bool(config->metric_enable));
	trace("config write buffer enable %s\n", human_bool(config->buffer_enable));
	trace("config write reading interval %hu\n", config->reading_interval);
	trace("config write metric interval %hu\n", config->metric_interval);
	trace("config write buffer interval %hu\n", config->buffer_interval);
	trace("config write frequency %u\n", config->frequency);
	trace("config write bandwidth %u\n", config->bandwidth);
	trace("config write coding rate %hhu\n", config->coding_rate);
	trace("config write spreading factor %hhu\n", config->spreading_factor);
	trace("config write preamble length %hhu\n", config->preamble_length);
	trace("config write tx power %hhu\n", config->tx_power);
	trace("config write sync word 0x%02x\n", config->sync_word);
	trace("config write checksum %s\n", human_bool(config->checksum));

	size_t offset = 0;
	uint8_t buffer[256];
	memcpy(&buffer[offset], config->id, sizeof(config->id));
	offset += sizeof(config->id);
	memcpy(&buffer[offset], config->firmware, sizeof(config->firmware));
	offset += sizeof(config->firmware);
	memcpy(&buffer[offset], config->hardware, sizeof(config->hardware));
	offset += sizeof(config->hardware);
	buffer[offset] = config->led_debug;
	offset += sizeof(config->led_debug);
	buffer[offset] = config->reading_enable;
	offset += sizeof(config->reading_enable);
	buffer[offset] = config->metric_enable;
	offset += sizeof(config->metric_enable);
	buffer[offset] = config->buffer_enable;
	offset += sizeof(config->buffer_enable);
	memcpy(&buffer[offset], (uint16_t[]){hton16(config->reading_interval)}, sizeof(config->reading_interval));
	offset += sizeof(config->reading_interval);
	memcpy(&buffer[offset], (uint16_t[]){hton16(config->metric_interval)}, sizeof(config->metric_interval));
	offset += sizeof(config->metric_interval);
	memcpy(&buffer[offset], (uint16_t[]){hton16(config->buffer_interval)}, sizeof(config->buffer_interval));
	offset += sizeof(config->buffer_interval);
	memcpy(&buffer[offset], (uint32_t[]){hton32(config->frequency)}, sizeof(config->frequency));
	offset += sizeof(config->frequency);
	memcpy(&buffer[offset], (uint32_t[]){hton32(config->bandwidth)}, sizeof(config->bandwidth));
	offset += sizeof(config->bandwidth);
	buffer[offset] = config->coding_rate;
	offset += sizeof(config->coding_rate);
	buffer[offset] = config->spreading_factor;
	offset += sizeof(config->spreading_factor);
	buffer[offset] = config->preamble_length;
	offset += sizeof(config->preamble_length);
	buffer[offset] = config->tx_power;
	offset += sizeof(config->tx_power);
	buffer[offset] = config->sync_word;
	offset += sizeof(config->sync_word);
	buffer[offset] = config->checksum;
	offset += sizeof(config->checksum);

	uint32_t irq = save_and_disable_interrupts();
	flash_range_erase(PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE, FLASH_SECTOR_SIZE);
	flash_range_program(PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE, buffer, FLASH_PAGE_SIZE);
	restore_interrupts(irq);
}

void config_read(config_t *config) {
	const uint8_t *base = (const uint8_t *)(XIP_BASE + PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE);

	size_t offset = 0;
	memcpy(config->id, &base[offset], sizeof(config->id));
	offset += sizeof(config->id);
	memcpy(config->firmware, &base[offset], sizeof(config->firmware));
	offset += sizeof(config->firmware);
	memcpy(config->hardware, &base[offset], sizeof(config->hardware));
	offset += sizeof(config->hardware);
	config->led_debug = base[offset];
	offset += sizeof(config->led_debug);
	config->reading_enable = base[offset];
	offset += sizeof(config->reading_enable);
	config->metric_enable = base[offset];
	offset += sizeof(config->metric_enable);
	config->buffer_enable = base[offset];
	offset += sizeof(config->buffer_enable);
	memcpy(&config->reading_interval, &base[offset], sizeof(config->reading_interval));
	config->reading_interval = (uint16_t)(ntoh16(config->reading_interval));
	offset += sizeof(config->reading_interval);
	memcpy(&config->metric_interval, &base[offset], sizeof(config->metric_interval));
	config->metric_interval = (uint16_t)(ntoh16(config->metric_interval));
	offset += sizeof(config->metric_interval);
	memcpy(&config->buffer_interval, &base[offset], sizeof(config->buffer_interval));
	config->buffer_interval = (uint16_t)(ntoh16(config->buffer_interval));
	offset += sizeof(config->buffer_interval);
	memcpy(&config->frequency, &base[offset], sizeof(config->frequency));
	config->frequency = (uint32_t)(ntoh32(config->frequency));
	offset += sizeof(config->frequency);
	memcpy(&config->bandwidth, &base[offset], sizeof(config->bandwidth));
	config->bandwidth = (uint32_t)(ntoh32(config->bandwidth));
	offset += sizeof(config->bandwidth);
	config->coding_rate = base[offset];
	offset += sizeof(config->coding_rate);
	config->spreading_factor = base[offset];
	offset += sizeof(config->spreading_factor);
	config->preamble_length = base[offset];
	offset += sizeof(config->preamble_length);
	config->tx_power = base[offset];
	offset += sizeof(config->tx_power);
	config->sync_word = base[offset];
	offset += sizeof(config->sync_word);
	config->checksum = base[offset];
	offset += sizeof(config->checksum);

	trace("config read id 0x%02x%02x\n", config->id[0], config->id[1]);
	trace("config read firmware 0x%02x%02x\n", config->firmware[0], config->firmware[1]);
	trace("config read hardware 0x%02x%02x\n", config->hardware[0], config->hardware[1]);
	trace("config read led debug %s\n", human_bool(config->led_debug));
	trace("config read reading enable %s\n", human_bool(config->reading_enable));
	trace("config read metric enable %s\n", human_bool(config->metric_enable));
	trace("config read buffer enable %s\n", human_bool(config->buffer_enable));
	trace("config read reading interval %hu\n", config->reading_interval);
	trace("config read metric interval %hu\n", config->metric_interval);
	trace("config read buffer interval %hu\n", config->buffer_interval);
	trace("config read frequency %u\n", config->frequency);
	trace("config read bandwidth %u\n", config->bandwidth);
	trace("config read coding rate %hhu\n", config->coding_rate);
	trace("config read spreading factor %hhu\n", config->spreading_factor);
	trace("config read preamble length %hhu\n", config->preamble_length);
	trace("config read tx power %hhu\n", config->tx_power);
	trace("config read sync word 0x%02x\n", config->sync_word);
	trace("config read checksum %s\n", human_bool(config->checksum));
}
