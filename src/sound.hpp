#ifndef SOUND_HPP_INCLUDED
#define SOUND_HPP_INCLUDED

#include "wml_node_fwd.hpp"

#include <string>

namespace sound {

struct manager {
	manager();
	~manager();
};

void init_music(wml::const_node_ptr node);

bool ok();
bool muted();
void mute(bool flag);

void process();

//preload a sound effect in the cache.
void preload(const std::string& file);

//play a sound. 'object' is the object that is playing the sound. It can be
//used later in stop_sound to specify which object is stopping playing
//the sound.
void play(const std::string& file, const void* object=0);

//stop a sound. object refers to the object that started the sound, and is
//the same as the object in play().
void stop_sound(const std::string& file, const void* object=0);

//stop all looped sounds associated with an object; same object as in play()
//intended to be called in all object's destructors
void stop_looped_sounds(const void* object=0);
void change_volume(const void* object=0, int volume=-1);

//Ways to set the sound and music volumes from the user's perspective.
float get_sound_volume();
void set_sound_volume(float volume);

float get_music_volume();
void set_music_volume(float volume);

//Ways to set the music volume from the game engine's perspective.
void set_engine_music_volume(float volume);
float get_engine_music_volume();
	
// function to play a sound effect over and over in a loop. Will return
// a handle to the sound effect.
int play_looped(const std::string& file, const void* object=0);

void play_music(const std::string& file);
void play_music_interrupt(const std::string& file);

const std::string& current_music();

}

#endif
