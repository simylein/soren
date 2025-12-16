#include <stdint.h>

uint16_t min16(uint16_t alpha, uint16_t bravo) {
	if (alpha < bravo) {
		return alpha;
	}
	return bravo;
}

uint16_t sub16(uint16_t alpha, uint16_t bravo) {
	if (alpha < bravo) {
		return 0;
	}
	return alpha - bravo;
}

uint32_t min32(uint32_t alpha, uint32_t bravo) {
	if (alpha < bravo) {
		return alpha;
	}
	return bravo;
}

uint32_t sub32(uint32_t alpha, uint32_t bravo) {
	if (alpha < bravo) {
		return 0;
	}
	return alpha - bravo;
}
