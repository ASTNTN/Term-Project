#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <time.h>

#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
#include <linux/nl80211.h>

#include <detector/config.h>
#include <detector/datagram.h>
#include <detector/thread.h>
#include <detector/time.h>

static inline struct datagram build_datagram(void) {
	static uint64_t generation_counter = 0;

	struct datagram datagram = {
		.generation = generation_counter++,
		.timestamp = time_nanoseconds()
	};

	return datagram;
}

int main(int argc, const char **argv) {
	thread_setup(THREAD_NUMBER_CLIENT);

	if (argc != 2) {
		fputs("Usage: client <address>\n", stderr);
		exit(EXIT_FAILURE);
	}

	const char *address = argv[1];

	int sd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sd < 0) {
		perror("CLIENT ERROR: socket failed");
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in sockaddr = {0};
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons(RECEIVER_PORT);
	if (inet_pton(AF_INET, address, &sockaddr.sin_addr) != 1) {
		perror("CLIENT ERROR: inet_pton failed");
		exit(EXIT_FAILURE);
	}

	for (;;) {
		struct timespec next;
		clock_gettime(CLOCK_MONOTONIC, &next);

		struct datagram datagram = build_datagram();

		// Client and server are both little-endian
		if (sendto(sd, &datagram, sizeof(datagram), 0, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) < 0) {
			perror("CLIENT ERROR: sendto failed");
			exit(EXIT_FAILURE);
		}

		next.tv_nsec += INTERVAL_NANOSECONDS;
		next.tv_sec += next.tv_nsec / 1000000000L;
		next.tv_nsec %= 1000000000L;

		clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL);
	}

	return 0;
}