/*
 * keyd - A key remapping daemon.
 *
 * © 2019 Raheman Vaiya (see also: LICENSE).
 */
#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "keyd.h"
#include "keys.h"
#include "unicode.h"
#include "config.h"
#include "device.h"
#include <memory>

#define MAX_ACTIVE_KEYS	32
#define CACHE_SIZE	16 //Effectively nkro

struct keyboard;

struct cache_entry {
	uint8_t code;
	struct descriptor d;
	int dl;
	int layer;
};

struct key_event {
	uint8_t code;
	uint8_t pressed;
	int timestamp;
};

struct output {
	void (*send_key) (uint8_t code, uint8_t state);
	void (*on_layer_change) (const struct keyboard *kbd, const struct layer *layer, uint8_t active);
};

enum class chord_state_e : signed char {
	CHORD_RESOLVING,
	CHORD_INACTIVE,
	CHORD_PENDING_DISAMBIGUATION,
	CHORD_PENDING_HOLD_TIMEOUT,
};

using enum chord_state_e;

enum class pending_behaviour_e : signed char {
	PK_INTERRUPT_ACTION1,
	PK_INTERRUPT_ACTION2,
	PK_UNINTERRUPTIBLE,
	PK_UNINTERRUPTIBLE_TAP_ACTION2,
};

using enum pending_behaviour_e;

struct active_chord {
	uint8_t active;
	struct chord chord;
	int layer;
};

/* May correspond to more than one physical input device. */
struct keyboard {
	std::vector<config_backup> original_config;
	struct config config;
	struct output output;

	/*
	 * Cache descriptors to preserve code->descriptor
	 * mappings in the event of mid-stroke layer changes.
	 */
	struct cache_entry cache[CACHE_SIZE];

	uint8_t last_pressed_output_code;
	uint8_t last_pressed_code;

	uint8_t oneshot_latch;

	uint8_t inhibit_modifier_guard;

	::macro* active_macro;
	int active_macro_layer;
	int overload_last_layer_code;

	long macro_timeout;
	long oneshot_timeout;

	long macro_repeat_interval;

	long overload_start_time;

	long last_simple_key_time;

	long timeouts[64];
	size_t nr_timeouts;

	struct active_chord active_chords[KEYD_CHORD_MAX-KEYD_CHORD_1+1];

	struct {
		struct key_event queue[32];
		size_t queue_sz;

		const struct chord *match;
		int match_layer;

		uint8_t start_code;
		long last_code_time;

		enum chord_state_e state;
	} chord;

	struct {
		uint8_t code;
		uint8_t dl;
		long expire;
		long tap_expiry;

		enum pending_behaviour_e behaviour;

		struct key_event queue[32];
		size_t queue_sz;

		struct descriptor action1;
		struct descriptor action2;
	} pending_key;

	struct layer_state_t {
		long activation_time;

		uint8_t active;
		uint8_t toggled;
		uint8_t oneshot_depth;
	};
	std::vector<layer_state_t> layer_state;

	uint8_t keystate[256];

	struct {
		int x;
		int y;

		int sensitivity; /* Mouse units per scroll unit (higher == slower scrolling). */
		int active;
	} scroll;
};

std::unique_ptr<keyboard> new_keyboard(std::unique_ptr<keyboard>);

long kbd_process_events(struct keyboard *kbd, const struct key_event *events, size_t n);
bool kbd_eval(struct keyboard *kbd, std::string_view);
void kbd_reset(struct keyboard *kbd);

#endif
