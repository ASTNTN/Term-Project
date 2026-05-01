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

enum segment_state {
	SEGMENT_FREE = 0, // can be written into
	SEGMENT_IN_FLIGHT // submitted to kernel
};

struct segment {
	struct entry entries[ENTRY_COUNT];
};

struct entries {
	struct segment segments[SEGMENT_COUNT];
	enum segment_state states[SEGMENT_COUNT];
	size_t entry_index;
	size_t segment_index;
	off_t write_offset;
};

static struct entries entries = {0};
static struct io_uring ring = {0};

// The idea is to fill a segment up. The server moves on to the next segment, and the kernel writes the existing segment to a file.
// There will only ever be one producer thread for this sink.

static inline void handle_completions(void) {
	struct io_uring_cqe *cqe;

	while (io_uring_peek_cqe(&ring, &cqe) == 0) {
		size_t seg_id = cqe->user_data;

		if (cqe->res < 0) {
			fprintf(stderr, "write failed: %s\n", strerror(-cqe->res));
			exit(EXIT_FAILURE);
		}

		entries.states[seg_id] = SEGMENT_FREE;

		io_uring_cqe_seen(&ring, cqe);
	}
}

static inline void submit_segment(int sink, off_t offset) {
	struct io_uring_sqe *sqe;
	while (!(sqe = io_uring_get_sqe(&ring))) {
		fprintf(stderr, "SERVER WARNING: SQE full\n");
		handle_completions();
	}

	size_t seg_id = entries.segment_index;
	struct segment *segment = &entries.segments[seg_id];

	io_uring_prep_write(sqe, sink, segment, sizeof(struct segment), offset);

	sqe->user_data = seg_id;

	entries.states[seg_id] = SEGMENT_IN_FLIGHT;

	int ret = io_uring_submit(&ring);
	if (ret < 0) {
		fprintf(stderr, "io_uring_submit failed: %s\n", strerror(-ret));
		exit(EXIT_FAILURE);
	}
}

static inline void write_entry(struct entry entry, int sink) {
	struct segment *segment = &entries.segments[entries.segment_index];

	segment->entries[entries.entry_index++] = entry;

	if (entries.entry_index < ENTRY_COUNT)
		return;

	submit_segment(sink, entries.write_offset);

	entries.entry_index = 0;

	entries.segment_index = (entries.segment_index + 1) % SEGMENT_COUNT;
	entries.write_offset += sizeof(struct segment);

	if (entries.states[entries.segment_index] != SEGMENT_FREE) {
		fprintf(stderr, "Next buffer still in flight - increase SEGMENT_COUNT\n");
		exit(EXIT_FAILURE);
	}
}

static inline int connect_sink(const char *address, bool file) {
	int sink;

	if (file) {
		sink = open(address, O_WRONLY | O_CREAT | O_TRUNC, 0644);
		if (sink < 0) {
			perror("SERVER ERROR: sink open failed");
			exit(EXIT_FAILURE);
		}
	}
	else {
		sink = socket(AF_INET, SOCK_STREAM, 0);
		if (sink < 0) {
			perror("SERVER ERROR: sink socket failed");
			exit(EXIT_FAILURE);
		}

		struct sockaddr_in sink_sockaddr;
		sink_sockaddr.sin_family = AF_INET;
		sink_sockaddr.sin_port = htons(RECEIVER_PORT);
		if (inet_pton(AF_INET, address, &sink_sockaddr.sin_addr) != 1) {
			perror("CLIENT ERROR: sink inet_pton failed");
			exit(EXIT_FAILURE);
		}

		if (connect(sink, (struct sockaddr *) &sink_sockaddr, sizeof(sink_sockaddr))) {
			perror("CLIENT ERROR: sink connect failed");
			exit(EXIT_FAILURE);
		}
	}

	return sink;
}

static inline int bind_source(void) {
	int source = socket(AF_INET, SOCK_DGRAM, 0);
	if (source < 0) {
		perror("SERVER ERROR: source socket failed");
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in source_sockaddr = {0};
	source_sockaddr.sin_family = AF_INET;
	source_sockaddr.sin_addr.s_addr = INADDR_ANY;
	source_sockaddr.sin_port = htons(RECEIVER_PORT);

	if (bind(source, (struct sockaddr *) &source_sockaddr, sizeof(source_sockaddr))) {
		perror("SERVER ERROR: source bind failed");
		exit(EXIT_FAILURE);
	}

	return source;
}

int main(int argc, char **argv) {
	thread_setup(THREAD_NUMBER_SERVER);

	bool file;

	if (argc == 2)
		file = false;
	else if (argc == 3)
		file = true;
	else {
		fputs("Usage: server <address>\n", stderr);
		exit(EXIT_SUCCESS);
	}

	int sink = connect_sink(argv[1], file);

	if (io_uring_queue_init(SEGMENT_COUNT, &ring, 0) < 0) {
		perror("ERROR: io_uring_queue_init failed");
		exit(EXIT_FAILURE);
	}

	int source = bind_source();

	uint64_t generation = 0;

	double average_latency = 0;
	uint64_t count_latency = 0;
	uint64_t count_dropped = 0;
	uint64_t count_duplicate = 0;

	for (;;) {
		struct datagram datagram;

		// Client and server are both little-endian
		ssize_t size = recvfrom(source, &datagram, sizeof(datagram), 0, NULL, NULL);
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

    	write_entry(entry, sink);

			count_latency = 0;
			count_dropped = 0;
			count_duplicate = 0;
		}

		static int flush_counter = 0;
		if (++flush_counter >= 16) {
			flush_counter = 0;
    	handle_completions();
		}
	}

	return 0;
}