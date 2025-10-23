#include <stdint.h>

uint16_t min(uint16_t alpha, uint16_t bravo) {
	if (alpha < bravo) {
		return alpha;
	}
	return bravo;
}

uint16_t sub(uint16_t alpha, uint16_t bravo) {
	if (alpha < bravo) {
		return 0;
	}
	return alpha - bravo;
}
