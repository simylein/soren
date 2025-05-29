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
	gpio_pull_down(sx1278_dio0_pin);
}

uint8_t sx1278_read_register(uint8_t reg) {
	uint8_t tx_buf[2] = {reg & 0x7f, 0x00};
	uint8_t rx_buf[2];

	gpio_put(sx1278_nss_pin, 0);
	spi_write_read_blocking(sx1278_spi, tx_buf, rx_buf, 2);
	gpio_put(sx1278_nss_pin, 1);

	return rx_buf[1];
}
