/*
 * keyd - A key remapping daemon.
 *
 * © 2019 Raheman Vaiya (see also: LICENSE).
 */
#ifndef CONFIG_H
#define CONFIG_H

#include <limits.h>
#include "macro.h"
#include <memory>
#include <vector>
#include <string>
#include <string_view>
#include <array>
#include <map>

#define MAX_DESCRIPTOR_ARGS	3

#define ID_EXCLUDED	1
#define ID_MOUSE	2
#define ID_KEYBOARD	4
#define ID_ABS_PTR	8

enum class op : signed char {
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

using enum op;

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
	uint8_t id;
	enum op op;
	union descriptor_arg args[MAX_DESCRIPTOR_ARGS];
};

static_assert(sizeof(descriptor) == 8);

// Experimental flat map with deferred sorting for layer keymap descriptors
struct descriptor_map {
	size_t size = 0;
	std::array<descriptor, 3> maps{};
	std::vector<descriptor> mapv; // Should be empty by default

	void sort();
	void set(uint8_t id, const descriptor& copy, uint8_t hint = 48);
	const descriptor& operator[](uint8_t id) const;
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

enum class layer_type_e : signed char {
	LT_NORMAL,
	LT_LAYOUT,
	LT_COMPOSITE,
};

using enum layer_type_e;

struct layer {
	std::string name;

	enum layer_type_e type;
	bool modified = false; // Modified by kbd_eval
	uint8_t mods;
	std::vector<chord> chords;
	descriptor_map keymap;

	/* Used for composite layers. */
	size_t nr_constituents;
	int constituents[8];
};

struct env_pack {
	std::vector<char> buf;
	std::unique_ptr<const char*[]> env;
	uid_t uid;
	gid_t gid;

	const char* getenv(std::string_view);
};

struct ucmd {
	uid_t uid;
	gid_t gid;
	std::string cmd;
	std::shared_ptr<env_pack> env = nullptr;
};

struct dev_id {
	uint8_t flags;
	std::array<char, 23> id;
};

struct config {
	std::string pathstr;
	std::vector<layer> layers;
	std::map<std::string, size_t, std::less<>> layer_names;

	/* Auxiliary descriptors used by layer bindings. */
	std::vector<descriptor> descriptors;
	std::vector<macro> macros;
	std::vector<ucmd> commands;
	std::multimap<std::string, descriptor, std::less<>> aliases;

	uid_t cfg_use_uid = 0;
	gid_t cfg_use_gid = 0;
	std::shared_ptr<env_pack> env;

	std::vector<dev_id> ids;

	long macro_timeout;
	long macro_sequence_timeout;
	long macro_repeat_timeout;
	long oneshot_timeout;

	long overload_tap_timeout;

	long chord_interkey_timeout;
	long chord_hold_timeout;

	uint8_t wildcard = 0;
	uint8_t layer_indicator = 255;
	uint8_t disable_modifier_guard;
	std::string default_layout;

	config() = default;
	config(const config&) = delete;
	config& operator=(const config&) = delete;
	~config();
};

struct config_backup {
	struct layer_backup {
		decltype(layer::keymap) keymap;
		decltype(layer::chords) chords;
	};

	// These are append-only
	size_t descriptor_count;
	size_t macro_count;
	size_t cmd_count;
	// These ones are nasty
	std::vector<layer_backup> layers;

	explicit config_backup(const struct config& cfg);
	~config_backup();

	void restore(struct config& cfg);
};

int config_parse(struct config *config, const char *path);
int config_add_entry(struct config *config, std::string_view);

int config_check_match(struct config *config, const char *id, uint8_t flags);

#endif
