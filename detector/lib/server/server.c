#define _GNU_SOURCE

#include <detector/server.h>

#include <arpa/inet.h>
#include <liburing.h>
#include <math.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <detector/config.h>
#include <detector/datagram.h>
#include <detector/thread.h>
#include <detector/time.h>

struct entry {
	double latency;
	uint64_t dropped;
	uint64_t duplicate;
};

static struct io_uring ring;
static struct entry entries[SEGMENT_COUNT][SEGMENT_SIZE];
static size_t segment_index;
static size_t entry_index;

static inline void submit_entry(int sink) {
	struct io_uring_sqe *sqe = io_uring_get_sqe(&ring);
	if (!sqe) {
		io_uring_submit(&ring);
		sqe = io_uring_get_sqe(&ring);
		if (!sqe) return;
	}

	io_uring_prep_write(sqe, sink, &entries[segment_index], sizeof(entries[segment_index]), -1);

	sqe->flags = IOSQE_ASYNC;
}

void *server_main(void *sink_void) {
	thread_setup(THREAD_NUMBER_SERVER);

	if (!sink_void) {
		fputs("SERVER ERROR: Sink is NULL\n", stderr);
		exit(EXIT_FAILURE);
	}

	int sink = *(int *)sink_void;

	if (io_uring_queue_init(SUBMISSION_QUEUE_ENTRY_COUNT, &ring, 0) < 0) {
		perror("io_uring_queue_init failed");
		exit(EXIT_FAILURE);
	}

	int sd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sd < 0) {
		perror("SERVER ERROR: socket failed");
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in sockaddr = {0};
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_addr.s_addr = INADDR_ANY;
	sockaddr.sin_port = htons(RECEIVER_PORT);

	if (bind(sd, (struct sockaddr *) &sockaddr, sizeof(sockaddr))) {
		perror("SERVER ERROR: bind failed");
		exit(EXIT_FAILURE);
	}

	uint64_t generation = 0;

	double average_latency = 0;
	uint64_t count_latency = 0;
	uint64_t count_dropped = 0;
	uint64_t count_duplicate = 0;

	for (;;) {
		struct datagram datagram;

		// Client and server are both little-endian
		ssize_t size = recvfrom(sd, &datagram, sizeof(datagram), 0, NULL, NULL);
		if (size < 0) {
			perror("SERVER ERROR: recvfrom failed");
			exit(EXIT_FAILURE);
		}
		else if (size != sizeof(datagram)) {
			fprintf(stderr, "SERVER WARNING: Unexpected datagram size (%zd)\n", size);
			continue;
		}

		if (datagram.generation <= generation) {
			++count_duplicate;
			continue;
		}

		count_dropped += datagram.generation - generation - 1;
		generation = datagram.generation;

		++count_latency;
		double sample = (double)(time_nanoseconds() - datagram.timestamp);
		average_latency += (sample - average_latency) / count_latency;

		if (count_latency == LATENCY_GROUPING) {
			struct entry entry = {
				.latency = average_latency,
				.dropped = count_dropped,
				.duplicate = count_duplicate,
			};

			count_latency = 0;
			count_dropped = 0;
			count_duplicate = 0;

			entries[++entry_index][segment_index] = entry;
		}

		if (segment_index == SEGMENT_SIZE) {
			submit_entry(sink);
			segment_index = segment_index + 1 % SEGMENT_COUNT;
			entry_index = 0;
		}
	}

	return NULL;
}