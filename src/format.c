#include <stdbool.h>

const char *human_bool(bool val) {
	if (val) {
		return "true";
	}
	return "false";
}
