#ifndef MACRO_H
#define MACRO_H

#include <stdint.h>
#include <stdlib.h>

enum macro_e {
	MACRO_KEYSEQUENCE,
	MACRO_HOLD,
	MACRO_RELEASE,
	MACRO_UNICODE,
	MACRO_TIMEOUT
};

struct macro_entry {
	enum macro_e type;

	uint16_t data;
};

/*
 * A series of key sequences optionally punctuated by
 * timeouts
 */
struct macro {
	struct macro_entry entries[64];

	uint32_t sz;
};


void macro_execute(void (*output)(uint8_t, uint8_t),
		   const struct macro *macro,
		   size_t timeout);

int macro_parse(char *s, struct macro *macro);
#endif
