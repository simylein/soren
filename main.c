#include "pico/stdlib.h"
#include "rp2040.h"
#include "si7021.h"
#include <stdio.h>

int main(void) {
  rp2040_stdio_init();
  rp2040_led_init();
  si7021_init();

  while (true) {
    rp2040_led_blink(10, 50);

    float temperature = si7021_temperature();
    float humidity = si7021_humidity();
    printf("temperature %.2f humidity %.2f\n", temperature, humidity);

    sleep_ms(10000);
  }
}
