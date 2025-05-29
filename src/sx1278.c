#include <hardware/spi.h>
#include <pico/stdlib.h>
#include <stdio.h>

spi_inst_t *sx1278_spi = spi0;

const uint sx1278_miso_pin = 16;
const uint sx1278_nss_pin = 17;
const uint sx1278_scl_pin = 18;
const uint sx1278_mosi_pin = 19;
const uint sx1278_dio0_pin = 20;

void sx1278_init(void) {
	printf("sx1278: initialising gpio %d %d %d %d and %d\n", sx1278_miso_pin, sx1278_nss_pin, sx1278_scl_pin, sx1278_mosi_pin,
				 sx1278_dio0_pin);

	spi_init(sx1278_spi, 1 * 1000 * 1000);

	gpio_set_function(sx1278_scl_pin, GPIO_FUNC_SPI);
	gpio_set_function(sx1278_mosi_pin, GPIO_FUNC_SPI);
	gpio_set_function(sx1278_miso_pin, GPIO_FUNC_SPI);

	gpio_init(sx1278_nss_pin);
	gpio_init(sx1278_dio0_pin);

	gpio_set_dir(sx1278_nss_pin, GPIO_OUT);
	gpio_set_dir(sx1278_dio0_pin, GPIO_IN);

	gpio_put(sx1278_nss_pin, 1);
}

void sx1278_write_reg(uint8_t reg, uint8_t value) {
	gpio_put(sx1278_nss_pin, 0);
	spi_write_blocking(sx1278_spi, (uint8_t[]){reg | 0x80, value}, 2);
	gpio_put(sx1278_nss_pin, 1);
}

uint8_t sx1278_read_register(uint8_t reg) {
	uint8_t tx_buf[2] = {reg & 0x7f, 0x00};
	uint8_t rx_buf[2];

	gpio_put(sx1278_nss_pin, 0);
	spi_write_read_blocking(sx1278_spi, tx_buf, rx_buf, 2);
	gpio_put(sx1278_nss_pin, 1);

	return rx_buf[1];
}

void sx1278_lora(void) { sx1278_write_reg(0x01, 0x81); }

void sx1278_sleep(void) { sx1278_write_reg(0x01, 0x00); }

void sx1278_frequency(uint32_t freq_hz) {
	uint32_t frf = (uint64_t)freq_hz * (1 << 19) / (32 * 1000 * 1000);

	sx1278_write_reg(0x06, (frf >> 16) & 0xff);
	sx1278_write_reg(0x07, (frf >> 8) & 0xff);
	sx1278_write_reg(0x08, frf & 0xff);

	printf("set frequency to %u hz and frf 0x%6x\n", freq_hz, frf);
}

void sx1278_tx_power(uint8_t power_dbm) {
	if (power_dbm < 2) {
		power_dbm = 2;
	} else if (power_dbm > 17) {
		power_dbm = 17;
	}

	uint8_t pa_config = 0x80 | (power_dbm - 2);
	sx1278_write_reg(0x09, pa_config);

	printf("set tx power to %d dbm pa_config 0x%2x\n", power_dbm, pa_config);
}

void sx1278_coding_rate(uint8_t cr) {
	if (cr < 5) {
		cr = 5;
	} else if (cr > 8) {
		cr = 8;
	}

	uint8_t modem_config_1 = sx1278_read_register(0x1d);

	modem_config_1 &= ~(0x0e);
	uint8_t cr_bits = (cr - 4) << 1;
	modem_config_1 |= cr_bits;

	sx1278_write_reg(0x1d, modem_config_1);

	printf("set coding rate to 4/%d modem_config_1 0x%2x\n", cr, modem_config_1);
}

void sx1278_bandwidth(uint32_t bandwidth_hz) {
	uint8_t bw_bits;

	switch (bandwidth_hz) {
	case 7800:
		bw_bits = 0x0;
		break;
	case 10400:
		bw_bits = 0x1;
		break;
	case 15600:
		bw_bits = 0x2;
		break;
	case 20800:
		bw_bits = 0x3;
		break;
	case 31250:
		bw_bits = 0x4;
		break;
	case 41700:
		bw_bits = 0x5;
		break;
	case 62500:
		bw_bits = 0x6;
		break;
	case 125000:
		bw_bits = 0x7;
		break;
	case 250000:
		bw_bits = 0x8;
		break;
	case 500000:
		bw_bits = 0x9;
		break;
	default:
		bw_bits = 0x5;
		break;
	}

	uint8_t modem_config_1 = sx1278_read_register(0x1d);

	modem_config_1 = (modem_config_1 & 0x0f) | (bw_bits << 4);

	sx1278_write_reg(0x1d, modem_config_1);

	printf("set bandwidth to %u hz bw_bits 0x%1x modem_config_1 0x%2x\n", bandwidth_hz, bw_bits, modem_config_1);
}

void sx1278_spreading_factor(uint8_t sf) {
	if (sf < 7) {
		sf = 7;
	} else if (sf > 12) {
		sf = 12;
	}

	uint8_t modem_config_2 = sx1278_read_register(0x1e);

	modem_config_2 &= 0x0f;
	modem_config_2 |= (sf << 4);

	sx1278_write_reg(0x1e, modem_config_2);

	printf("set spreading factor to sf %d modem_config_2 0x%2x\n", sf, modem_config_2);
}

void sx1278_send(const uint8_t *data, uint8_t length, uint16_t timeout_ms) {
	sx1278_write_reg(0x01, 0x81);

	sx1278_write_reg(0x0d, 0x80);
	sx1278_write_reg(0x0e, 0x80);

	sx1278_write_reg(0x40, 0x00);

	gpio_put(sx1278_nss_pin, 0);
	uint8_t header = 0x80;
	spi_write_blocking(sx1278_spi, &header, sizeof(header));
	spi_write_blocking(sx1278_spi, data, length);
	gpio_put(sx1278_nss_pin, 1);

	sx1278_write_reg(0x22, length);
	sx1278_write_reg(0x01, 0x83);

	printf("sending data %.*s length %d\n", length, data, length);

	uint16_t time_spent = 0;
	while (true) {
		if (sx1278_read_register(0x12) & 0x08) {
			printf("sending completed successfully\n");
			break;
		}
		sleep_ms(10);
		time_spent += 10;
		if (time_spent >= timeout_ms) {
			printf("send timeout %d ms reached\n", timeout_ms);
			break;
		}
	}

	sx1278_write_reg(0x12, 0xff);
}
