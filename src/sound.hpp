#ifndef SOUND_HPP_INCLUDED
#define SOUND_HPP_INCLUDED

#include <string>

namespace sound {

struct manager {
	manager();
	~manager();
};

bool ok();
bool muted();
void mute(bool flag);

void play(const std::string& file);

// function to play a sound effect over and over in a loop. Will return
// a handle to the sound effect. Will keep playing until cancel_looped()
// is called with the handle.
int play_looped(const std::string& file);
void cancel_looped(int handle);

void play_music(const std::string& file);
void play_music_interrupt(const std::string& file);

}

#endif
