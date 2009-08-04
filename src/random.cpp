#include "random.hpp"

namespace rng {

static unsigned int next = 1;

int generate() {
	next = next * 1103515245 + 12345;
	return((unsigned int)(next/65536) % 32768);
}

void set_seed(unsigned int seed) {
	next = seed;
}

unsigned int get_seed() {
	return next;
}

}
