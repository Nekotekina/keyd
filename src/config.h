/*
 * keyd - A key remapping daemon.
 *
 * © 2019 Raheman Vaiya (see also: LICENSE).
 */
#ifndef CONFIG_H
#define CONFIG_H

#include <limits.h>
#include "macro.h"
#include <vector>
#include <string>
#include <array>

#define MAX_DESCRIPTOR_ARGS	3

#define ID_EXCLUDED	1
#define ID_MOUSE	2
#define ID_KEYBOARD	4

enum op {
	OP_NULL = 0,
	OP_KEYSEQUENCE = 1,

	OP_ONESHOT,
	OP_ONESHOTM,
	OP_LAYERM,
	OP_SWAP,
	OP_SWAPM,
	OP_LAYER,
	OP_LAYOUT,
	OP_CLEAR,
	OP_CLEARM,
	OP_OVERLOAD,
	OP_OVERLOAD_TIMEOUT,
	OP_OVERLOAD_TIMEOUT_TAP,
	OP_OVERLOAD_IDLE_TIMEOUT,
	OP_TOGGLE,
	OP_TOGGLEM,

	OP_MACRO,
	OP_MACRO2,
	OP_COMMAND,
	OP_TIMEOUT,

/* Experimental */
	OP_SCROLL_TOGGLE,
	OP_SCROLL,
};

union descriptor_arg {
	uint8_t code;
	uint8_t mods;
	int16_t idx;
	uint16_t sz;
	uint16_t timeout;
	int16_t sensitivity;
};

/* Describes the intended purpose of a key (corresponds to an 'action' in user parlance). */

struct descriptor {
	enum op op;
	union descriptor_arg args[MAX_DESCRIPTOR_ARGS];
};

struct chord {
	std::array<uint8_t, 8> keys;
	struct descriptor d;
};

/*
 * A layer is a map from keycodes to descriptors. It may optionally
 * contain one or more modifiers which are applied to the base layout in
 * the event that no matching descriptor is found in the keymap. For
 * consistency, modifiers are internally mapped to eponymously named
 * layers consisting of the corresponding modifier and an empty keymap.
 */

enum layer_type_e : short {
	LT_NORMAL,
	LT_LAYOUT,
	LT_COMPOSITE,
};

struct layer {
	std::string name;

	enum layer_type_e type;

	uint8_t mods;
	std::vector<chord> chords;
	struct descriptor keymap[256];

	/* Used for composite layers. */
	size_t nr_constituents;
	int constituents[8];
};

struct config {
	std::string pathstr;
	std::vector<layer> layers;

	/* Auxiliary descriptors used by layer bindings. */
	std::vector<descriptor> descriptors;
	std::vector<macro> macros;
	std::vector<std::string> commands;
	std::array<std::string, 256> aliases;

	uint8_t wildcard;
	struct {
		char id[64];
		uint8_t flags;
	} ids[64];

	size_t nr_ids;

	long macro_timeout;
	long macro_sequence_timeout;
	long macro_repeat_timeout;
	long oneshot_timeout;

	long overload_tap_timeout;

	long chord_interkey_timeout;
	long chord_hold_timeout;

	uint8_t layer_indicator;
	uint8_t disable_modifier_guard;
	std::string default_layout;
};

int config_parse(struct config *config, const char *path);
int config_add_entry(struct config *config, const char *exp);

int config_check_match(struct config *config, const char *id, uint8_t flags);

#endif
