#ifndef SOUND_HPP_INCLUDED
#define SOUND_HPP_INCLUDED

#include <string>

namespace sound {

struct manager {
	manager();
	~manager();
};

bool ok();

void play(const std::string& file);
void play_music(const std::string& file);

}

#endif
