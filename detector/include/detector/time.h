#ifndef TIME_H
#define TIME_H

#include <stdint.h>
#include <time.h>

#define NANOSECONDS_PER_SECOND (1000 * 1000 * 1000)

static inline uint64_t time_nanoseconds(void ) {
	struct timespec timespec_now = {0};
	clock_gettime(CLOCK_MONOTONIC, &timespec_now);

	return (uint64_t) timespec_now.tv_sec * NANOSECONDS_PER_SECOND + (uint64_t) timespec_now.tv_nsec;
}

#endif