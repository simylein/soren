#include <hardware/spi.h>
#include <pico/stdlib.h>
#include <stdio.h>

const bool sx1278_debug = false;

spi_inst_t *sx1278_spi = spi0;
const uint sx1278_spi_speed = 1 * 1000 * 1000;

const uint sx1278_miso_pin = 16;
const uint sx1278_nss_pin = 17;
const uint sx1278_scl_pin = 18;
const uint sx1278_mosi_pin = 19;
const uint sx1278_dio0_pin = 20;

void sx1278_init(void) {
	if (sx1278_debug) {
		printf("sx1278: initialising gpio %d %d %d %d and %d\n", sx1278_miso_pin, sx1278_nss_pin, sx1278_scl_pin, sx1278_mosi_pin,
					 sx1278_dio0_pin);
	}

	spi_init(sx1278_spi, sx1278_spi_speed);

	gpio_set_function(sx1278_scl_pin, GPIO_FUNC_SPI);
	gpio_set_function(sx1278_mosi_pin, GPIO_FUNC_SPI);
	gpio_set_function(sx1278_miso_pin, GPIO_FUNC_SPI);

	gpio_init(sx1278_nss_pin);
	gpio_init(sx1278_dio0_pin);

	gpio_set_dir(sx1278_nss_pin, GPIO_OUT);
	gpio_set_dir(sx1278_dio0_pin, GPIO_IN);
}

uint8_t sx1278_read_register(uint8_t reg) {
	uint8_t tx_buf[2] = {reg & 0x7f, 0x00};
	uint8_t rx_buf[2];

	gpio_put(sx1278_nss_pin, 0);
	if (spi_write_read_blocking(sx1278_spi, tx_buf, rx_buf, sizeof(rx_buf)) != sizeof(rx_buf)) {
		printf("sx1278: failed to read register 0x%02x\n", reg);
	}
	gpio_put(sx1278_nss_pin, 1);

	return rx_buf[1];
}

void sx1278_write_register(uint8_t reg, uint8_t value) {
	uint8_t tx_buf[2] = {reg | 0x80, value};

	gpio_put(sx1278_nss_pin, 0);
	if (spi_write_blocking(sx1278_spi, tx_buf, sizeof(tx_buf)) != sizeof(tx_buf)) {
		printf("sx1278: failed to write register 0x%02x\n", reg);
	}
	gpio_put(sx1278_nss_pin, 1);
}

void sx1278_sleep(void) {
	uint16_t time_us = 0;
	sx1278_write_register(0x01, 0x80);
	while ((sx1278_read_register(0x01) & 0x07) != 0x00) {
		sleep_us(50);
		time_us += 50;
		if (time_us > 2000) {
			printf("sx1278: timeout reached waiting for sleep mode\n");
			break;
		}
	}
	if (sx1278_debug) {
		printf("sx1278: sleep op_mode 0x%02x\n", sx1278_read_register(0x01));
	}
}

void sx1278_standby(void) {
	uint16_t time_us = 0;
	sx1278_write_register(0x01, 0x81);
	while ((sx1278_read_register(0x01) & 0x07) != 0x01) {
		sleep_us(50);
		time_us += 50;
		if (time_us > 2000) {
			printf("sx1278: timeout reached waiting for standby mode\n");
			break;
		}
	}
	if (sx1278_debug) {
		printf("sx1278: standby op_mode 0x%02x\n", sx1278_read_register(0x01));
	}
}

void sx1278_transfer(void) {
	uint16_t time_us = 0;
	sx1278_write_register(0x01, 0x83);
	while ((sx1278_read_register(0x01) & 0x07) != 0x03) {
		sleep_us(50);
		time_us += 50;
		if (time_us > 2000) {
			printf("sx1278: timeout reached waiting for transfer mode\n");
			break;
		}
	}
	if (sx1278_debug) {
		printf("sx1278: transfer op_mode 0x%02x\n", sx1278_read_register(0x01));
	}
}

void sx1278_receive(void) {
	uint16_t time_us = 0;
	sx1278_write_register(0x01, 0x85);
	while ((sx1278_read_register(0x01) & 0x07) != 0x05) {
		sleep_us(50);
		time_us += 50;
		if (time_us > 2000) {
			printf("sx1278: timeout reached waiting for receive mode\n");
			break;
		}
	}
	if (sx1278_debug) {
		printf("sx1278: receive op_mode 0x%02x\n", sx1278_read_register(0x01));
	}
}

void sx1278_frequency(uint32_t frequency) {
	uint32_t frf = (uint64_t)frequency * (1 << 19) / (32 * 1000 * 1000);

	sx1278_write_register(0x06, (frf >> 16) & 0xff);
	sx1278_write_register(0x07, (frf >> 8) & 0xff);
	sx1278_write_register(0x08, frf & 0xff);
	if (sx1278_debug) {
		printf("sx1278: frequency %u frf 0x%02x%02x%02x\n", frequency, sx1278_read_register(0x06), sx1278_read_register(0x07),
					 sx1278_read_register(0x08));
	}
}

void sx1278_tx_power(uint8_t power) {
	if (power < 2) {
		power = 2;
	} else if (power > 17) {
		power = 17;
	}

	uint8_t pa_config = 0x80 | (power - 2);

	sx1278_write_register(0x09, pa_config);
	if (sx1278_debug) {
		printf("sx1278: tx power %hhu pa_config 0x%02x\n", power, sx1278_read_register(0x09));
	}
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

	sx1278_write_register(0x1d, modem_config_1);
	if (sx1278_debug) {
		printf("sx1278: coding rate 4/%d modem_config_1 0x%02x\n", cr, sx1278_read_register(0x1d));
	}
}

void sx1278_bandwidth(uint32_t bandwidth) {
	uint8_t bw_bits;

	switch (bandwidth) {
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

	sx1278_write_register(0x1d, modem_config_1);
	if (sx1278_debug) {
		printf("sx1278: bandwidth %u modem_config_1 0x%02x\n", bandwidth, sx1278_read_register(0x1d));
	}
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

	sx1278_write_register(0x1e, modem_config_2);
	if (sx1278_debug) {
		printf("sx1278: spreading factor %d modem_config_2 0x%02x\n", sf, sx1278_read_register(0x1e));
	}
}

void sx1278_checksum(bool crc) {
	uint8_t modem_config_2 = sx1278_read_register(0x1e);

	if (crc) {
		modem_config_2 |= 0x04;
	} else {
		modem_config_2 &= ~0x04;
	}

	sx1278_write_register(0x1e, modem_config_2);
	if (sx1278_debug) {
		printf("sx1278: checksum %s modem_config_2 0x%02x\n", crc ? "true" : "false", sx1278_read_register(0x1e));
	}
}

void sx1278_send(uint8_t (*data)[256], uint8_t length, uint16_t timeout_ms) {
	sx1278_write_register(0x0d, 0x80);
	sx1278_write_register(0x0e, 0x80);

	sx1278_write_register(0x40, 0x00);

	gpio_put(sx1278_nss_pin, 0);
	uint8_t header = 0x80;
	spi_write_blocking(sx1278_spi, &header, sizeof(header));
	spi_write_blocking(sx1278_spi, *data, length);
	gpio_put(sx1278_nss_pin, 1);

	sx1278_transfer();
	sx1278_write_register(0x22, length);

	if (sx1278_debug) {
		printf("sx1278: sending data ");
		for (uint8_t ind = 0; ind < length; ind++) {
			printf("%02x", (*data)[ind]);
		}
		printf(" length %d\n", length);
	}

	uint32_t time_us = 0;
	while (true) {
		if (sx1278_read_register(0x12) & 0x08) {
			if (sx1278_debug) {
				printf("sx1278: sending completed %.02f ms irq_flags 0x%02x\n", (float)time_us / 1000, sx1278_read_register(0x12));
			}
			break;
		}
		sleep_us(50);
		time_us += 50;
		if (time_us / 1000 >= timeout_ms) {
			printf("sx1278: send timeout %hu ms reached irq_flags 0x%02x\n", timeout_ms, sx1278_read_register(0x12));
			break;
		}
	}

	sx1278_write_register(0x12, 0xff);
	if (sx1278_debug) {
		printf("sx1278: acknowledge flags irq_flags 0x%02x\n", sx1278_read_register(0x12));
	}
}

uint8_t sx1278_recv(uint8_t (*data)[256], uint8_t *length, uint16_t timeout_ms) {
	sx1278_receive();

	uint32_t time_us = 0;
	while (true) {
		if (sx1278_read_register(0x12) & 0x40) {
			if (sx1278_debug) {
				printf("sx1278: receiving completed %.02f ms irq_flags 0x%02x\n", (float)time_us / 1000, sx1278_read_register(0x12));
			}
			break;
		}
		sleep_us(50);
		time_us += 50;
		if (time_us / 1000 >= timeout_ms) {
			printf("sx1278: recv timeout %hu ms reached irq_flags 0x%02x\n", timeout_ms, sx1278_read_register(0x12));
			break;
		}
	}

	uint8_t rx_addr = sx1278_read_register(0x10);
	uint8_t packet_len = sx1278_read_register(0x13);

	sx1278_write_register(0x0d, rx_addr);

	for (uint8_t index = 0; index < packet_len; index++) {
		(*data)[index] = sx1278_read_register(0x00);
	}

	*length = packet_len;

	if (sx1278_debug) {
		printf("sx1278: received data ");
		for (uint8_t ind = 0; ind < *length; ind++) {
			printf("%02x", (*data)[ind]);
		}
		printf(" length %d\n", *length);
	}

	sx1278_write_register(0x12, 0xff);
	if (sx1278_debug) {
		printf("sx1278: acknowledge flags irq_flags 0x%02x\n", sx1278_read_register(0x12));
	}
}
