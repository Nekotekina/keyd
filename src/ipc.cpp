/*
 * keyd - A key remapping daemon.
 *
 * © 2019 Raheman Vaiya (see also: LICENSE).
 */

#include "keyd.h"

/* TODO (maybe): settle on an API and publish the protocol. */

#ifndef SOCKET_PATH
#define SOCKET_PATH "/var/run/keyd.socket"
#endif

static void chgid()
{
	struct group gb{};
	struct group* g = nullptr;
	{
		file_mapper groups(open("/etc/group", O_RDONLY));
		for (auto str : split_char<'\n'>(groups.view())) {
			if (str.starts_with("keyd:x:")) {
				g = &gb;
				gb.gr_gid = atoi(str.data() + 7);
				break;
			}
		}
	}

	if (!g) {
		// Attempt to use dummy stack buffer, then fallback to getgrnam if it fails
		char buf[1024];
		if (getgrnam_r("keyd", &gb, buf, sizeof(buf) - 1, &g) < 0 || !g) {
			perror("getgrnam_r");
			g = getgrnam("keyd");
		}
	}

	if (!g) {
		perror("getgrnam");
		fprintf(stderr,
			"WARNING: failed to set effective group to \"keyd\" (make sure the group exists)\n");
	} else {
		if (setgid(g->gr_gid)) {
			perror("setgid");
			exit(-1);
		}
	}
}

int ipc_connect()
{
	int sd = socket(AF_UNIX, SOCK_STREAM, 0);
	struct sockaddr_un addr = {};

	if (sd < 0) {
		perror("socket");
		exit(-1);
	}

	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path)-1);

	if (connect(sd, (struct sockaddr *) &addr, sizeof addr) < 0) {
		fprintf(stderr, "ERROR: Failed to connect to \"" SOCKET_PATH "\", make sure the daemon is running and you have permission to access the socket.\n");
		exit(-1);
	}

	return sd;
}

int ipc_create_server()
{
	int sd = socket(AF_UNIX, SOCK_STREAM, 0);
	int lfd;
	struct sockaddr_un addr = {};

	chgid();

	if (sd < 0) {
		perror("socket");
		exit(-1);
	}
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path)-1);
	lfd = open(SOCKET_PATH ".lock", O_CREAT | O_RDONLY, 0600);

	if (lfd < 0) {
		perror("open");
		exit(-1);
	}

	if (flock(lfd, LOCK_EX | LOCK_NB))
		return -1;

	unlink(SOCKET_PATH);
	if (bind(sd, (struct sockaddr *) &addr, sizeof addr) < 0) {
		fprintf(stderr, "failed to bind to socket %s\n", SOCKET_PATH);
		exit(-1);
	}

	if (listen(sd, 20) < 0) {
		perror("listen");
		exit(-1);
	}

	chmod(SOCKET_PATH, 0660);

	return sd;
}
