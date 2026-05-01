#ifndef DATAGRAM_H
#define DATAGRAM_H

#include <stdint.h>

struct datagram {
	uint64_t generation;
	uint64_t timestamp;
};

#endif