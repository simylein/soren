#include "buffer.h"
#include "logger.h"
#include <string.h>

buffer_t buffer = {
		.head = 0,
		.tail = 0,
		.size = 0,
};

void buffer_push(uplink_t *uplink) {
	buffer.uplinks[buffer.tail].kind = uplink->kind;
	memcpy(&buffer.uplinks[buffer.tail].data, uplink->data, uplink->data_len);
	buffer.uplinks[buffer.tail].data_len = uplink->data_len;
	buffer.uplinks[buffer.tail].captured_at = uplink->captured_at;

	buffer.tail = (uint16_t)((buffer.tail + 1) % (sizeof(buffer.uplinks) / sizeof(uplink_t)));
	if (buffer.size < sizeof(buffer.uplinks) / sizeof(uplink_t)) {
		buffer.size++;
	}
	trace("increased buffer size to %hu\n", buffer.size);
}

void buffer_peek(uplink_t *uplink) {
	uplink->kind = buffer.uplinks[buffer.head].kind;
	memcpy(uplink->data, buffer.uplinks[buffer.head].data, buffer.uplinks[buffer.head].data_len);
	uplink->data_len = buffer.uplinks[buffer.head].data_len;
	uplink->captured_at = buffer.uplinks[buffer.head].captured_at;
}

void buffer_pop() {
	buffer.head = (uint16_t)((buffer.head + 1) % (sizeof(buffer.uplinks) / sizeof(uplink_t)));
	if (buffer.size > 0) {
		buffer.size--;
	}
	trace("decreased buffer size to %hu\n", buffer.size);
}
