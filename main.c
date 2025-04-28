#include "hardware/adc.h"
#include "pico/stdlib.h"
#include <stdio.h>

const uint LED_PIN = 25;
const uint VSYS_PIN = 29;

int main() {
  stdio_init_all();

  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);

  adc_init();
  adc_gpio_init(VSYS_PIN);

  while (true) {
    gpio_put(LED_PIN, 1);
    sleep_ms(50);
    gpio_put(LED_PIN, 0);
    sleep_ms(950);

    uint16_t raw_value = adc_read();
    float voltage = (raw_value / 4095.0) * 3.3;
    printf("vsys %.3fv\n", voltage);
  }
}
