#pragma once

#include "config.h"
#include <pico/types.h>
#include <stdint.h>
#include <time.h>

typedef struct uplink_t {
	uint8_t kind;
	uint8_t data[256];
	uint8_t data_len;
	datetime_t captured_at;
} uplink_t;

int transceive(config_t *config, uplink_t *uplink);
