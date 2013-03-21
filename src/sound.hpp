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
#ifndef SOUND_HPP_INCLUDED
#define SOUND_HPP_INCLUDED

#include "variant.hpp"

#include <string>

namespace sound {

struct manager {
	manager();
	~manager();
};

void init_music(variant node);

bool ok();
bool muted();
void mute(bool flag);

void process();

//preload a sound effect in the cache.
void preload(const std::string& file);

//play a sound. 'object' is the object that is playing the sound. It can be
//used later in stop_sound to specify which object is stopping playing
//the sound.
void play(const std::string& file, const void* object=0, float volume=1.0f, float fade_in_time_=0.0f);

//stop a sound. object refers to the object that started the sound, and is
//the same as the object in play().
void stop_sound(const std::string& file, const void* object=0, float fade_out_time=0.0f);

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
int play_looped(const std::string& file, const void* object=0, float volume=1.0f, float fade_in_time_=0.0f);

void play_music(const std::string& file);
void play_music_interrupt(const std::string& file);

const std::string& current_music();

}

#endif
