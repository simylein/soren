#include "logger.h"
#include <pico/stdlib.h>

void rp2040_stdio_init(void) {
	trace("initialising stdio\n");
	stdio_init_all();

	sleep_ms(2000);
}
