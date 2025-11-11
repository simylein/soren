#pragma once

#include "config.h"
#include <stdint.h>
#include <time.h>

typedef struct uplink_t {
	uint8_t kind;
	uint8_t data[32];
	uint8_t data_len;
	time_t captured_at;
} uplink_t;

int configure(config_t *config);

int transceive(config_t *config, uplink_t *uplink);
