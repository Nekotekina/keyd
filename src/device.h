/*
 * keyd - A key remapping daemon.
 *
 * © 2019 Raheman Vaiya (see also: LICENSE).
 */
#ifndef DEVICE_H
#define DEVICE_H

#include <stdint.h>
#include <array>

#define CAP_MOUSE	0x1
#define CAP_MOUSE_ABS	0x2
#define CAP_KEYBOARD	0x4
#define CAP_LEDS	0x8

struct device {
	/*
	 * A file descriptor that can be used to monitor events subsequently read with
	 * device_read_event().
	 */
	int fd;

	uint8_t grabbed : 1;
	uint8_t is_virtual : 1;
	uint8_t capabilities : 6;

	char id[23];
	uint32_t num;
	char name[96];

	uint8_t led_state[LED_CNT];

	/* Internal. */
	uint32_t _maxx;
	uint32_t _maxy;
	uint32_t _minx;
	uint32_t _miny;

	/* Reserved for the user. */
	void *data;
};

enum class dev_event_e : signed char {
	DEV_KEY,
	DEV_LED,

	DEV_MOUSE_MOVE,
	/* All absolute values are relative to a resolution of 1024x1024. */
	DEV_MOUSE_MOVE_ABS,
	DEV_MOUSE_SCROLL,

	DEV_REMOVED,
};

using enum dev_event_e;

struct device_event {
	enum dev_event_e type;
	uint8_t pressed;
	uint16_t code;
	uint32_t x;
	uint32_t y;
};


struct device_event *device_read_event(struct device *dev);

void device_scan(std::vector<device>& devices);
int device_grab(struct device *dev);
int device_ungrab(struct device *dev);

int devmon_create();
int devmon_read_device(int fd, struct device *dev);
void device_set_led(const struct device *dev, uint8_t led, int state);

#endif
