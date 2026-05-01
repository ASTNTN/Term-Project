#ifndef THREAD_H
#define THREAD_H

#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

static inline void thread_setup(int cpu) {
	int ret;

	struct sched_param sp = { .sched_priority = 50 };
	ret = pthread_setschedparam(pthread_self(), SCHED_FIFO, &sp);
	if (ret) {
		fprintf(stderr, "ERROR: pthread_setschedparam failed: %s\n", strerror(ret));
		exit(EXIT_FAILURE);
	}

	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(cpu, &cpuset);

	ret = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
	if (ret) {
		fprintf(stderr, "ERROR: pthread_setaffinity_np failed: %s\n", strerror(ret));
		exit(EXIT_FAILURE);
	}

	if (mlockall(MCL_CURRENT | MCL_FUTURE)) {
		perror("mlockall failed");
		exit(EXIT_FAILURE);
	}
}

#endif