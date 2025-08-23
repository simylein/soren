#include <stdint.h>

uint16_t hton16(uint16_t value) { return (value >> 8) | (value << 8); }

uint16_t ntoh16(uint16_t value) { return (value >> 8) | (value << 8); }

uint32_t hton32(uint32_t value) {
	return ((value & 0x000000ffu) << 24) | ((value & 0x0000ff00u) << 8) | ((value & 0x00ff0000u) >> 8) |
				 ((value & 0xff000000u) >> 24);
}

uint32_t ntoh32(uint32_t value) {
	return ((value & 0x000000ffu) << 24) | ((value & 0x0000ff00u) << 8) | ((value & 0x00ff0000u) >> 8) |
				 ((value & 0xff000000u) >> 24);
}
