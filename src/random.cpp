/*
	Copyright (C) 2003-2013 by David White <davewx7@gmail.com>
	
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
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
