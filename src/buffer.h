#pragma once

#include "app.h"
#include <stdint.h>

typedef struct buffer_t {
	uplink_t uplinks[512];
	uint16_t head;
	uint16_t tail;
	uint16_t size;
} buffer_t;

extern buffer_t buffer;

void buffer_push(uplink_t *uplink);
void buffer_peek(uplink_t *uplink);
void buffer_pop();
