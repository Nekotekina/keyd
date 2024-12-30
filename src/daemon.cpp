#include "keyd.h"
#include "log.h"
#include <memory>
#include <utility>

#ifndef CONFIG_DIR
#define CONFIG_DIR ""
#endif

struct config_ent {
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
extern std::vector<device> device_table;

static std::array<uint8_t, KEY_CNT> keystate{};

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

	for (i = 0; i <= KEY_MAX; i++)
		if (keystate[i]) {
			vkbd_send_key(vkbd.get(), i, 0);
			keystate[i] = 0;
		}
}

static void send_key(uint16_t code, uint8_t state)
{
	keystate.at(code) = state;
	vkbd_send_key(vkbd.get(), code, state);
}

static void add_listener(::listener con)
{
	struct timeval tv;

	/*
	 * In order to avoid blocking the main event loop, allow up to 50ms for
	 * slow clients to relieve back pressure before dropping them.
	 */
	tv.tv_usec = 50000;
	tv.tv_sec = 0;

	setsockopt(con, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof tv);

	if (active_kbd) {
		struct config *config = &active_kbd->config;
		struct layer *layout = &config->layers[0];

		for (size_t i = 1; i < config->layers.size(); i++)
			if (active_kbd->layer_state[i].active) {
				struct layer *layer = &config->layers[i];

				if (layer->type == LT_LAYOUT) {
					layout = layer;
					break;
				}
			}

		dprintf(con, "/%s\n", layout->name.c_str());

		for (size_t i = 0; i < config->layers.size(); i++) {
			if (active_kbd->layer_state[i].active) {
				ssize_t ret;
				struct layer *layer = &config->layers[i];

				if (layer->type != LT_LAYOUT) {
					ret = dprintf(con, "+%s\n", layer->name.c_str());
					if (ret < 0)
						return;
				}
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

	for (size_t i = 0; i < device_table.size(); i++)
		if (device_table[i].data == kbd && (device_table[i].capabilities & CAP_LEDS)) {
			if (std::exchange(device_table[i].led_state[ind], active_layers) == active_layers)
				continue;
			device_set_led(&device_table[i], ind, active_layers);
		}
}

static void restore_leds()
{
	for (size_t i = 0; i < device_table.size(); i++) {
		struct device* dev = &device_table[i];
		if (dev->grabbed && dev->data && (dev->capabilities & CAP_LEDS)) {
			for (int j = 0; j < LED_CNT; j++) {
				device_set_led(dev, dev->led_state[j], j);
			}
		}
	}
}

static void on_layer_change(const struct keyboard *kbd, struct layer *layer, uint8_t state)
{
	if (kbd->config.layer_indicator) {
		activate_leds(kbd);
	}

	char c = '/';
	if (layer->type != LT_LAYOUT)
		c = state ? '+' : '-';

	std::erase_if(listeners, [&](::listener& listener) {
		return dprintf(listener, "%c%s\n", c, layer->name.c_str()) < 0;
	});
}

static void load_configs()
{
	DIR *dh = opendir(CONFIG_DIR);

	if (!dh) {
		perror("opendir");
		exit(-1);
	}

	configs.reset();

	while (struct dirent* dirent = readdir(dh)) {
		if (dirent->d_type == DT_DIR)
			continue;

		auto name = concat(CONFIG_DIR "/", dirent->d_name);
		if (name.ends_with(".conf")) {
			auto ent = std::make_unique<config_ent>();

			keyd_log("CONFIG: parsing b{%s}\n", name.c_str());

			auto kbd = std::make_unique<keyboard>();
			if (!config_parse(&kbd->config, name.c_str())) {
				kbd->output = {
					.send_key = send_key,
					.on_layer_change = on_layer_change,
				};
				kbd = new_keyboard(std::move(kbd));
				kbd->original_config.reserve(2);
				kbd->original_config.emplace_back(kbd->config);

				ent->kbd = std::move(kbd);
				ent->next = std::move(configs);
				configs = std::move(ent);
			} else {
				keyd_log("DEVICE: y{WARNING} failed to parse %s\n", name.c_str());
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
		int r = config_check_match(&ent->kbd->config, id, flags);

		if (r > rank) {
			match = ent;
			rank = r;
		}

		ent = ent->next.get();
	}

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
	if (dev->capabilities & CAP_MOUSE_ABS)
		flags |= ID_ABS_PTR;

	if ((ent = lookup_config_ent(dev->id, flags))) {
		if (device_grab(dev)) {
			keyd_log("DEVICE: y{WARNING} Failed to grab /dev/input/%u\n", dev->num);
			dev->data = NULL;
			return;
		}

		keyd_log("DEVICE: g{match}    %s  %s\t(%s)\n",
			  dev->id, ent->kbd->config.pathstr.c_str(), dev->name);

		dev->data = ent->kbd.get();
		if (dev->capabilities & CAP_LEDS)
			device_set_led(dev, ent->kbd->config.layer_indicator, 0);
	} else {
		dev->data = NULL;
		device_ungrab(dev);
		keyd_log("DEVICE: r{ignoring} %s  (%s)\n", dev->id, dev->name);
	}
}

static void reload(std::shared_ptr<env_pack> env)
{
	restore_leds();
	load_configs();

	for (size_t i = 0; i < device_table.size(); i++)
		manage_device(&device_table[i]);

	clear_vkbd();

	if (env && env->uid >= 1000) {
		// Load user bindings (may be not loaded when executed as root)
		std::string buf;
		if (auto v = env->getenv("XDG_CONFIG_HOME"))
			buf.assign(v) += "/";
		else if (auto v = env->getenv("HOME"))
			buf.assign(v) += "/.config/";
		else
			buf.clear();
		buf += "keyd/bindings.conf";
		buf = file_reader(open(buf.c_str(), O_RDONLY), 4096, [&]{
			keyd_log("Unable to open %s\n", buf.c_str());
			perror("open");
		});

		if (buf.empty())
			return;

		for (auto ent = configs.get(); ent; ent = ent->next.get()) {
			ent->kbd->config.cfg_use_uid = env->uid;
			ent->kbd->config.cfg_use_gid = env->gid;
			ent->kbd->config.env = env;
			for (auto str : split_char<'\n'>(buf)) {
				if (str.empty())
					continue;
				if (!kbd_eval(ent->kbd.get(), str))
					keyd_log("Invalid binding: %.*s\n", (int)str.size(), str.data());
			}
			ent->kbd->original_config.emplace_back(ent->kbd->config);
		}
	}

	for (auto ent = configs.get(); ent; ent = ent->next.get()) {
		for (auto& layer : ent->kbd->config.layers)
			layer.keymap.sort();
	}
}

static void send_success(int con)
{
	struct ipc_message msg = {};

	msg.type = IPC_SUCCESS;
	msg.sz = 0;

	xwrite(con, &msg, sizeof msg);
}

static void send_fail(int con, const char *fmt, ...)
{
	struct ipc_message msg = {};
	va_list args;

	va_start(args, fmt);

	msg.type = IPC_FAIL;
	msg.sz = vsnprintf(msg.data, sizeof(msg.data), fmt, args);

	xwrite(con, &msg, sizeof msg);

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
			uint16_t code;
			uint8_t mods;
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

static bool handle_message(::listener& con, struct config* config, std::shared_ptr<env_pack> env)
{
	struct ipc_message msg;
	if (!xread(con, &msg, sizeof(msg))) {
		// Disconnected
		return false;
	}

	if (msg.sz >= sizeof(msg.data)) {
		send_fail(con, "maximum message size exceeded");
		return false;
	}
	msg.data[msg.sz] = 0;

	if (msg.timeout > 1000000) {
		send_fail(con, "timeout cannot exceed 1000 ms");
		return false;
	}

	::macro macro;
	switch (msg.type) {
		int success;

	case IPC_MACRO:
		while (msg.sz && msg.data[msg.sz-1] == '\n')
			msg.data[--msg.sz] = 0;

		if (macro_parse(msg.data, macro, config)) {
			send_fail(con, "%s", errstr);
			break;
		}

		macro_execute(send_key, macro, msg.timeout, config);
		send_success(con);

		break;
	case IPC_INPUT:
		if (input(msg.data, msg.sz, msg.timeout))
			send_fail(con, "%s", errstr);
		else
			send_success(con);
		break;
	case IPC_RELOAD:
		reload(std::move(env));
		send_success(con);
		break;
	case IPC_LAYER_LISTEN:
		add_listener(std::move(con));
		return false;
	case IPC_BIND:
		success = 0;

		if (msg.sz == sizeof(msg.data)) {
			send_fail(con, "bind expression size exceeded");
			break;
		}

		msg.data[msg.sz] = 0;

		for (auto ent = configs.get(); ent; ent = ent->next.get()) {
			ent->kbd->config.cfg_use_uid = config->cfg_use_uid;
			ent->kbd->config.cfg_use_gid = config->cfg_use_gid;
			ent->kbd->config.env = config->env;
			success |= kbd_eval(ent->kbd.get(), msg.data);
		}

		if (success)
			send_success(con);
		else
			send_fail(con, "%s", errstr);

		// Repeat
		return true;
	default:
		send_fail(con, "Unknown command");
		break;
	}

	return false;
}

static void handle_client(::listener con)
{
	static socklen_t ucred_len = sizeof(struct ucred);
	struct ucred cred{};
	if (getsockopt(con, SOL_SOCKET, SO_PEERCRED, &cred, &ucred_len) < 0)
		return;

	::config ephemeral_config;
	ephemeral_config.cfg_use_gid = cred.gid;
	ephemeral_config.cfg_use_uid = cred.uid;
	std::shared_ptr<env_pack> prev = nullptr;
	if (!prev || prev->uid != cred.uid || prev->gid != cred.gid)
		prev.reset();
	if (getuid() < 1000)
	{
		// Copy initial environment variables from caller process
		std::vector<char> buf = file_reader(open(concat("/proc/", cred.pid, "/environ").c_str(), O_RDONLY), 8192, [] {
			perror("environ");
		});
		if (!buf.empty()) {
			if (prev && prev->buf == buf) {
				// Share previous environment variables
				ephemeral_config.env = prev;
			} else {
				size_t count = std::count(buf.begin(), buf.end(), 0);
				auto env = std::make_unique<const char*[]>(count + 1);
				auto ptr = env.get();
				for (auto str : split_char<0>({buf.data(), buf.size() - 1}))
					*ptr++ = str.data();
				*ptr = nullptr;
				prev = ephemeral_config.env = std::make_shared<env_pack>(::env_pack{
					.buf = std::move(buf),
					.env = std::move(env),
					.uid = cred.uid,
					.gid = cred.gid,
				});
			}
		}
	}

	size_t msg_count = 0;
	while (handle_message(con, &ephemeral_config, prev ? prev : std::make_shared<env_pack>())) {
		ephemeral_config.commands.clear();
		msg_count++;
	}
	dbg2("%zu messages processed", msg_count);
}

static int event_handler(struct event *ev)
{
	static int64_t last_time = 0;
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
			/*
			 * Propagate LED events received by the virtual device from userspace
			 * to all grabbed devices.
			 */
			for (size_t i = 0; i < device_table.size(); i++) {
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
			::listener con(accept(ipcfd, NULL, 0));
			if (con < 0) {
				perror("accept");
				exit(-1);
			}

			handle_client(std::move(con));
		}
		break;
	default:
		break;
	}

	return timeout;
}

#ifndef VERSION
#define VERSION "unknown"
#endif

int run_daemon(int, char *[])
{
	ipcfd = ipc_create_server();
	if (ipcfd < 0)
		die("failed to create socket (another instance already running?)");

	vkbd = vkbd_init(VKBD_NAME);

	setvbuf(stdout, NULL, _IOLBF, 0);
	setvbuf(stderr, NULL, _IOLBF, 0);

	if (nice(-20) == -1) {
		perror("nice");
		exit(-1);
	}

	evloop_add_fd(ipcfd);

	reload(std::make_shared<env_pack>());

	atexit(cleanup);

	keyd_log("Starting keyd " VERSION "\n");
	evloop(event_handler);

	return 0;
}
