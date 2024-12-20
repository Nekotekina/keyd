#ifndef MACRO_H
#define MACRO_H

#include <stdint.h>
#include <stdlib.h>
#include <vector>
#include <string_view>

enum macro_e : uint16_t {
	MACRO_KEYSEQUENCE,
	MACRO_HOLD,
	MACRO_RELEASE,
	MACRO_UNICODE,
	MACRO_TIMEOUT,
	MACRO_COMMAND,
};

struct macro_entry {
	enum macro_e type;

	uint16_t data;
};

/*
 * A series of key sequences optionally punctuated by
 * timeouts
 */
using macro = std::vector<macro_entry>;

void macro_execute(void (*output)(uint8_t, uint8_t),
		   const macro& macro,
		   size_t timeout, struct config* config);

int macro_parse(std::string_view, macro& macro, struct config* config);
#endif
