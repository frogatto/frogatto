#include <iostream>

#include <time.h>

#include "random.hpp"

namespace rng {

static unsigned int UninitSeed = 11483;
static unsigned int next = UninitSeed;

int generate() {
	if(next == UninitSeed) {
		next = time(NULL);
	}

	next = next * 1103515245 + 12345;
	const int result = ((unsigned int)(next/65536) % 32768);
	return result;
}

void set_seed(unsigned int seed) {
	std::cerr << "RANDOM SEED: " << seed << "\n";
	next = seed;
}

unsigned int get_seed() {
	return next;
}

}
