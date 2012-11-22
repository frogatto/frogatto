#ifndef CHECKSUM_HPP_INCLUDED
#define CHECKSUM_HPP_INCLUDED

#include <string>

namespace checksum {

struct manager {
	manager();
	~manager();
};

const std::string& game_signature();
bool is_verified();
void verify_file(const std::string& fname, const std::string& contents);

}

#endif
