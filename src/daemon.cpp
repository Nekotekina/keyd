#include "keyd.h"
#include <memory>
#include <utility>

#ifndef CONFIG_DIR
#define CONFIG_DIR ""
#endif

struct config_ent {
	struct config config = {};
	std::unique_ptr<keyboard> kbd;
	std::unique_ptr<config_ent> next;

	config_ent() = default;
	config_ent(const config_ent&) = delete;
	config_ent& operator=(const config_ent&) = delete;
	~config_ent()
	{
		while (std::unique_ptr<config_ent> ent = std::move(next))
			next = std::move(ent->next);
	}
};

static int ipcfd = -1;
static std::shared_ptr<struct vkbd> vkbd;
static std::unique_ptr<config_ent> configs;

static uint8_t keystate[256];

struct listener
{
	listener() = default;
	explicit listener(int fd)
		: fd(fd)
	{
	}

	listener(const listener&) = delete;
	listener& operator=(const listener&) = delete;

	listener(listener&& r)
	{
		if (fd != -1)
			close(fd);
		std::swap(fd, r.fd);
		r.fd = -1;
	}

	listener& operator=(listener&& r)
	{
		std::swap(fd, r.fd);
		return *this;
	}

	~listener()
	{
		if (fd != -1)
			close(fd);
	}

	operator int() const
	{
		return fd;
	}

private:
	int fd = -1;
};

static std::vector<listener> listeners = [] {
	std::vector<listener> v;
	v.reserve(32);
	return v;
}();

static struct keyboard *active_kbd = NULL;

static void cleanup()
{
	configs.reset();
	vkbd.reset();
}

static void clear_vkbd()
{
	size_t i;

	for (i = 0; i < 256; i++)
		if (keystate[i]) {
			vkbd_send_key(vkbd.get(), i, 0);
			keystate[i] = 0;
		}
}

static void send_key(uint8_t code, uint8_t state)
{
	keystate[code] = state;
	vkbd_send_key(vkbd.get(), code, state);
}

static void add_listener(int con)
{
	struct timeval tv;

	/*
	 * In order to avoid blocking the main event loop, allow up to 50ms for
	 * slow clients to relieve back pressure before dropping them.
	 */
	tv.tv_usec = 50000;
	tv.tv_sec = 0;

	setsockopt(con, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof tv);

	::listener listener(con);

	if (active_kbd) {
		struct config *config = &active_kbd->config;

		for (size_t i = 0; i < config->layers.size(); i++) {
			if (active_kbd->layer_state[i].active) {
				struct layer *layer = &config->layers[i];

				if (layer->type != LT_LAYOUT)
					layer->name_buf[0] = '+'; // Should be already +
				if (size_t r = write(listener, layer->name_buf.c_str(), layer->name_buf.size()); r != layer->name_buf.size())
					return;
			}
		}
	}
	listeners.emplace_back(std::move(con));
}

static void activate_leds(const struct keyboard *kbd)
{
	int ind = kbd->config.layer_indicator;
	if (ind > LED_MAX)
		return;

	int active_layers = 0;

	for (size_t i = 1; i < kbd->config.layers.size(); i++)
		if (kbd->config.layers[i].type != LT_LAYOUT && kbd->layer_state[i].active) {
			active_layers = 1;
			break;
		}

	for (size_t i = 0; i < device_table_sz; i++)
		if (device_table[i].data == kbd && (device_table[i].capabilities & CAP_LEDS)) {
			if (std::exchange(device_table[i].led_state[ind], active_layers) == active_layers)
				continue;
			device_set_led(&device_table[i], ind, active_layers);
		}
}

static void restore_leds()
{
	for (size_t i = 0; i < device_table_sz; i++) {
		struct device* dev = &device_table[i];
		if (dev->grabbed && dev->data && (dev->capabilities & CAP_LEDS)) {
			for (int j = 0; j < LED_CNT; j++) {
				device_set_led(dev, dev->led_state[j], j);
			}
		}
	}
}

static void on_layer_change(const struct keyboard *kbd, const struct layer *layer, uint8_t state)
{
	if (kbd->config.layer_indicator <= LED_MAX) {
		activate_leds(kbd);
	}

	if (layer->type != LT_LAYOUT)
		layer->name_buf[0] = state ? '+' : '-';

	std::erase_if(listeners, [&](::listener& listener) {
		size_t nw = write(listener, layer->name_buf.c_str(), layer->name_buf.size());
		return nw != layer->name_buf.size();
	});
}

static void load_configs()
{
	DIR *dh = opendir(CONFIG_DIR);
	struct dirent *dirent;

	if (!dh) {
		perror("opendir");
		exit(-1);
	}

	configs.reset();

	while ((dirent = readdir(dh))) {
		char path[1024];
		int len;

		if (dirent->d_type == DT_DIR)
			continue;

		len = snprintf(path, sizeof path, "%s/%s", CONFIG_DIR, dirent->d_name);

		if (len >= 5 && !strcmp(path + len - 5, ".conf")) {
			auto ent = std::make_unique<config_ent>();

			keyd_log("CONFIG: parsing b{%s}\n", path);

			if (!config_parse(&ent->config, path)) {
				struct output output = {
					.send_key = send_key,
					.on_layer_change = on_layer_change,
				};
				ent->kbd = new_keyboard(&ent->config, &output);

				ent->next = std::move(configs);
				configs = std::move(ent);
			} else {
				keyd_log("DEVICE: y{WARNING} failed to parse %s\n", path);
			}

		}
	}

	closedir(dh);
}

static struct config_ent *lookup_config_ent(const char *id, uint8_t flags)
{
	struct config_ent *ent = configs.get();
	struct config_ent *match = NULL;
	int rank = 0;

	while (ent) {
		int r = config_check_match(&ent->config, id, flags);

		if (r > rank) {
			match = ent;
			rank = r;
		}

		ent = ent->next.get();
	}

	/* The wildcard should not match mice. */
	if (rank == 1 && (flags == ID_MOUSE))
		return NULL;
	else
		return match;
}

static void manage_device(struct device *dev)
{
	uint8_t flags = 0;
	struct config_ent *ent;

	if (dev->is_virtual)
		return;

	if (dev->capabilities & CAP_KEYBOARD)
		flags |= ID_KEYBOARD;
	if (dev->capabilities & (CAP_MOUSE|CAP_MOUSE_ABS))
		flags |= ID_MOUSE;

	if ((ent = lookup_config_ent(dev->id, flags))) {
		if (device_grab(dev)) {
			keyd_log("DEVICE: y{WARNING} Failed to grab %s\n", dev->path);
			dev->data = NULL;
			return;
		}

		keyd_log("DEVICE: g{match}    %s  %s\t(%s)\n",
			  dev->id, ent->config.pathstr.c_str(), dev->name);

		dev->data = ent->kbd.get();
		if (dev->capabilities & CAP_LEDS)
			device_set_led(dev, ent->kbd->config.layer_indicator, 0);
	} else {
		dev->data = NULL;
		device_ungrab(dev);
		keyd_log("DEVICE: r{ignoring} %s  (%s)\n", dev->id, dev->name);
	}
}

static void reload()
{
	restore_leds();
	configs.reset();
	load_configs();

	for (size_t i = 0; i < device_table_sz; i++)
		manage_device(&device_table[i]);

	clear_vkbd();
}

static void send_success(int con)
{
	struct ipc_message msg = {};

	msg.type = IPC_SUCCESS;
	msg.sz = 0;

	xwrite(con, &msg, sizeof msg);
	close(con);
}

static void send_fail(int con, const char *fmt, ...)
{
	struct ipc_message msg = {};
	va_list args;

	va_start(args, fmt);

	msg.type = IPC_FAIL;
	msg.sz = vsnprintf(msg.data, sizeof(msg.data), fmt, args);

	xwrite(con, &msg, sizeof msg);
	close(con);

	va_end(args);
}

static int input(char *buf, [[maybe_unused]] size_t sz, uint32_t timeout)
{
	size_t i;
	uint32_t codepoint;
	uint8_t codes[4];
	auto vkbd = ::vkbd.get();

	int csz;

	while ((csz = utf8_read_char(buf, &codepoint))) {
		int found = 0;
		char s[2];

		if (csz == 1) {
			uint8_t code, mods;
			s[0] = (char)codepoint;
			s[1] = 0;

			found = 1;
			if (!parse_key_sequence(s, &code, &mods)) {
				if (mods & MOD_SHIFT) {
					vkbd_send_key(vkbd, KEYD_LEFTSHIFT, 1);
					vkbd_send_key(vkbd, code, 1);
					vkbd_send_key(vkbd, code, 0);
					vkbd_send_key(vkbd, KEYD_LEFTSHIFT, 0);
				} else {
					vkbd_send_key(vkbd, code, 1);
					vkbd_send_key(vkbd, code, 0);
				}
			} else if ((char)codepoint == ' ') {
				vkbd_send_key(vkbd, KEYD_SPACE, 1);
				vkbd_send_key(vkbd, KEYD_SPACE, 0);
			} else if ((char)codepoint == '\n') {
				vkbd_send_key(vkbd, KEYD_ENTER, 1);
				vkbd_send_key(vkbd, KEYD_ENTER, 0);
			} else if ((char)codepoint == '\t') {
				vkbd_send_key(vkbd, KEYD_TAB, 1);
				vkbd_send_key(vkbd, KEYD_TAB, 0);
			} else {
				found = 0;
			}
		}

		if (!found) {
			int idx = unicode_lookup_index(codepoint);
			if (idx < 0) {
				err("ERROR: could not find code for \"%.*s\"", csz, buf);
				return -1;
			}

			unicode_get_sequence(idx, codes);

			for (i = 0; i < 4; i++) {
				vkbd_send_key(vkbd, codes[i], 1);
				vkbd_send_key(vkbd, codes[i], 0);
			}
		}
		buf+=csz;

		if (timeout)
			usleep(timeout);
	}

	return 0;
}

static void handle_client(int con)
{
	struct ipc_message msg;

	xread(con, &msg, sizeof msg);

	if (msg.sz >= sizeof(msg.data)) {
		send_fail(con, "maximum message size exceeded");
		return;
	}
	msg.data[msg.sz] = 0;

	if (msg.timeout > 1000000) {
		send_fail(con, "timeout cannot exceed 1000 ms");
		return;
	}

	::macro macro;
	switch (msg.type) {
		int success;

	case IPC_MACRO:
		while (msg.sz && msg.data[msg.sz-1] == '\n')
			msg.data[--msg.sz] = 0;

		if (macro_parse(msg.data, macro)) {
			send_fail(con, "%s", errstr);
			return;
		}

		macro_execute(send_key, macro, msg.timeout);
		send_success(con);

		break;
	case IPC_INPUT:
		if (input(msg.data, msg.sz, msg.timeout))
			send_fail(con, "%s", errstr);
		else
			send_success(con);
		break;
	case IPC_RELOAD:
		reload();
		send_success(con);
		break;
	case IPC_LAYER_LISTEN:
		add_listener(con);
		break;
	case IPC_BIND:
		success = 0;

		if (msg.sz == sizeof(msg.data)) {
			send_fail(con, "bind expression size exceeded");
			return;
		}

		msg.data[msg.sz] = 0;

		for (auto ent = configs.get(); ent; ent = ent->next.get()) {
			if (!kbd_eval(ent->kbd.get(), msg.data))
				success = 1;
		}

		if (success)
			send_success(con);
		else
			send_fail(con, "%s", errstr);


		break;
	default:
		send_fail(con, "Unknown command");
		break;
	}
}

static int event_handler(struct event *ev)
{
	static int last_time = 0;
	static int timeout = 0;
	struct key_event kev = {};
	auto vkbd = ::vkbd.get();

	timeout -= ev->timestamp - last_time;
	last_time = ev->timestamp;

	timeout = timeout < 0 ? 0 : timeout;

	switch (ev->type) {
	case EV_TIMEOUT:
		if (!active_kbd)
			return 0;

		kev.code = 0;
		kev.timestamp = ev->timestamp;

		timeout = kbd_process_events(active_kbd, &kev, 1);
		break;
	case EV_DEV_EVENT:
		if (ev->dev->data) {
			struct keyboard *kbd = (struct keyboard*)ev->dev->data;
			active_kbd = kbd;
			switch (ev->devev->type) {
			size_t i;
			case DEV_KEY:
				dbg("input %s %s", KEY_NAME(ev->devev->code), ev->devev->pressed ? "down" : "up");

				kev.code = ev->devev->code;
				kev.pressed = ev->devev->pressed;
				kev.timestamp = ev->timestamp;

				timeout = kbd_process_events(kbd, &kev, 1);
				break;
			case DEV_MOUSE_MOVE:
				if (kbd->scroll.active) {
					if (kbd->scroll.sensitivity == 0)
						break;
					int xticks, yticks;

					kbd->scroll.y += ev->devev->y;
					kbd->scroll.x += ev->devev->x;

					yticks = kbd->scroll.y / kbd->scroll.sensitivity;
					kbd->scroll.y %= kbd->scroll.sensitivity;

					xticks = kbd->scroll.x / kbd->scroll.sensitivity;
					kbd->scroll.x %= kbd->scroll.sensitivity;

					vkbd_mouse_scroll(vkbd, 0, -1*yticks);
					vkbd_mouse_scroll(vkbd, 0, xticks);
				} else {
					vkbd_mouse_move(vkbd, ev->devev->x, ev->devev->y);
				}
				break;
			case DEV_MOUSE_MOVE_ABS:
				vkbd_mouse_move_abs(vkbd, ev->devev->x, ev->devev->y);
				break;
			case DEV_LED:
				if (ev->devev->code <= LED_MAX) {
					ev->dev->led_state[ev->devev->code] = ev->devev->pressed;
					// Restore layer indicator state
					if (ev->devev->code == kbd->config.layer_indicator)
						activate_leds(kbd);
				}
				break;
			default:
				break;
			case DEV_MOUSE_SCROLL:
				/*
				 * Treat scroll events as mouse buttons so oneshot and the like get
				 * cleared.
				 */
				if (active_kbd) {
					kev.code = KEYD_EXTERNAL_MOUSE_BUTTON;
					kev.pressed = 1;
					kev.timestamp = ev->timestamp;

					kbd_process_events((struct keyboard*)ev->dev->data, &kev, 1);

					kev.pressed = 0;
					timeout = kbd_process_events((struct keyboard*)ev->dev->data, &kev, 1);
				}

				vkbd_mouse_scroll(vkbd, ev->devev->x, ev->devev->y);
				break;
			}
		} else if (ev->dev->is_virtual && ev->devev->type == DEV_LED) {
			size_t i;

			/*
			 * Propagate LED events received by the virtual device from userspace
			 * to all grabbed devices.
			 */
			for (i = 0; i < device_table_sz; i++) {
				::device& dev = device_table[i];
				if (dev.data && (dev.capabilities & CAP_LEDS)) {
					struct keyboard* kbd = (struct keyboard*)dev.data;
					if (ev->devev->code <= LED_MAX) {
						// Save LED state for restoring it later
						auto prev = std::exchange(dev.led_state[ev->devev->code], ev->devev->pressed);
						if (prev == ev->devev->pressed)
							continue;
					}
					if (ev->devev->code == kbd->config.layer_indicator) {
						// Suppress indicator change
						continue;
					}
					device_set_led(&dev, ev->devev->code, ev->devev->pressed);
				}

			}
			break;
		}

		break;
	case EV_DEV_ADD:
		manage_device(ev->dev);
		break;
	case EV_DEV_REMOVE:
		keyd_log("DEVICE: r{removed}\t%s %s\n", ev->dev->id, ev->dev->name);

		break;
	case EV_FD_ACTIVITY:
		if (ev->fd == ipcfd) {
			int con = accept(ipcfd, NULL, 0);
			if (con < 0) {
				perror("accept");
				exit(-1);
			}

			handle_client(con);
		}
		break;
	default:
		break;
	}

	return timeout;
}

int run_daemon(int, char *[])
{
	ipcfd = ipc_create_server(/* SOCKET_PATH */);
	if (ipcfd < 0)
		die("failed to create %s (another instance already running?)", SOCKET_PATH);

	vkbd = vkbd_init(VKBD_NAME);

	setvbuf(stdout, NULL, _IOLBF, 0);
	setvbuf(stderr, NULL, _IOLBF, 0);

	if (nice(-20) == -1) {
		perror("nice");
		exit(-1);
	}

	evloop_add_fd(ipcfd);

	reload();

	atexit(cleanup);

	keyd_log("Starting keyd " VERSION "\n");
	evloop(event_handler);

	return 0;
}
