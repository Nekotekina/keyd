/*
 * keyd - A key remapping daemon.
 *
 * © 2019 Raheman Vaiya (see also: LICENSE).
 */
#ifndef KEYD_H_
#define KEYD_H_

#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <grp.h>
#include <limits.h>
#include <poll.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/un.h>
#include <termios.h>
#include <getopt.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>

#ifdef __FreeBSD__
	#include <dev/evdev/input.h>
#else
	#include <linux/input.h>
#endif

#include "config.h"
#include "macro.h"
#include "device.h"
#include "log.h"
#include "keyboard.h"
#include "keys.h"
#include "vkbd.h"
#include "strutil.h"

#define MAX_IPC_MESSAGE_SIZE 4096

#define ARRAY_SIZE(x) (int)(sizeof(x)/sizeof(x[0]))
#define VKBD_NAME "keyd virtual "
#define C_SPACES " \f\n\r\t\v"

enum class event_type : signed char {
	EV_DEV_ADD,
	EV_DEV_REMOVE,
	EV_DEV_EVENT,
	EV_FD_ACTIVITY,
	EV_FD_ERR,
	EV_TIMEOUT,
};

using enum event_type;

struct event {
	enum event_type type;
	int fd;
	int64_t timestamp;
	struct device *dev;
	struct device_event *devev;
};

enum class ipc_msg_type_e : signed char {
	IPC_SUCCESS,
	IPC_FAIL,

	IPC_BIND,
	IPC_INPUT,
	IPC_MACRO,
	IPC_RELOAD,
	IPC_LAYER_LISTEN,
};

using enum ipc_msg_type_e;

struct ipc_message {
	enum ipc_msg_type_e type;

	uint32_t timeout;
	char data[MAX_IPC_MESSAGE_SIZE];
	size_t sz;
};

int monitor(int argc, char *argv[]);
int run_daemon(int argc, char *argv[]);

void evloop_add_fd(int fd);
int evloop(int (*event_handler)(struct event* ev), bool monitor = false);

void xwrite(int fd, const void *buf, size_t sz);
bool xread(int fd, void *buf, size_t sz);

int ipc_create_server();
int ipc_connect();

// One-time file reader
struct file_reader
{
	explicit file_reader(int fd, unsigned reserve, auto on_fail)
		: fd(fd)
		, reserve(reserve)
	{
		if (fd < 0) {
			on_fail();
		}
	}

	file_reader(const file_reader&) = delete;
	file_reader& operator=(const file_reader&) = delete;

	// Read full file
	template <typename T, typename V = typename T::value_type, typename = typename T::allocator_type>
	operator T()
	{
		static_assert(sizeof(V) == 1);
		T result;
		size_t rd = 0;
		if (fd < 0) [[unlikely]]
			return result;
		while (true) {
			const size_t new_size = rd + this->reserve;
			result.resize(new_size, 0);
			const auto rv = read(this->fd, result.data() + rd, new_size - rd);
			result.resize(rv < 0 ? 0 : (rd += rv));
			if (rv <= 0)
				break;
		}
		return result;
	}

	void reset()
	{
		if (fd >= 0 && lseek(fd, 0, SEEK_SET) < 0)
			perror("file_reader::lseek");
	}

	~file_reader()
	{
		if (fd >= 0)
			close(fd);
	}
private:
	int fd;
	unsigned reserve;
};

struct file_mapper
{
	explicit file_mapper(int fd)
	{
		if (fd < 0) {
			perror("fd");
			return;
		}

		if (auto seek = lseek(fd, 0, SEEK_END); seek < 0) {
			perror("lseek");
			close(fd);
			return;
		} else if (size_t(seek) > UINT32_MAX / 4) {
			close(fd);
			return;
		} else {
			this->size = seek;
		}

		this->ptr = ::mmap(nullptr, this->size + size_t(1), PROT_READ, MAP_SHARED, fd, 0);
		if (intptr_t(this->ptr) == -1) {
			perror("mmap");
			close(fd);
			return;
		}

		this->fd = fd;
	}

	explicit operator bool() const
	{
		return this->fd >= 0 && this->ptr;
	}

	std::string_view view() const
	{
		if (this->fd >= 0 && this->ptr)
			return {static_cast<char*>(this->ptr), this->size};
		return {};
	}

	file_mapper(const file_mapper&) = delete;
	file_mapper& operator=(const file_mapper&) = delete;

	~file_mapper()
	{
		if (this->fd >= 0) {
			::munmap(this->ptr, this->size + size_t(1));
			close(this->fd);
		}
	}
private:
	int fd = -1;
	unsigned size = 0;
	void* ptr = nullptr;
};

#endif
