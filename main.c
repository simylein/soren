#include "hardware/adc.h"
#include "hardware/timer.h"
#include "pico/sleep.h"
#include "pico/stdlib.h"
#include <stdio.h>

const uint LED_PIN = 25;
const uint VSYS_PIN = 29;

void wake(uint alarm) { printf("waking up from sleep\n"); }

int main(void) {
  stdio_init_all();
  sleep_ms(2000);

  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);

  adc_init();
  adc_gpio_init(VSYS_PIN);

  gpio_put(LED_PIN, 1);
  sleep_ms(50);
  gpio_put(LED_PIN, 0);

  while (true) {
    sleep_ms(1850);
    // sleep_goto_sleep_for(1850, &wake);

    gpio_put(LED_PIN, 1);
    sleep_ms(50);
    gpio_put(LED_PIN, 0);
    sleep_ms(50);
    gpio_put(LED_PIN, 1);
    sleep_ms(50);
    gpio_put(LED_PIN, 0);

    uint16_t raw_value = adc_read();
    float voltage = (raw_value / 4095.0) * 3.3;
    printf("vsys %.3fv\n", voltage);
  }
}
