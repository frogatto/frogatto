#include <iostream>
#include <map>
#include <vector>

#include "preferences.hpp"
#include "SDL.h"
#include "SDL_mixer.h"

#include "sound.hpp"

namespace sound {

namespace {
// number of allocated channels, 
const size_t NumChannels = 16;
const int SampleRate = 44100;

#ifdef WIN32
const size_t BufferSize = 4096;
#else
const size_t BufferSize = 1024;
#endif

bool sound_ok = false;
bool mute_ = false;

typedef std::map<std::string, Mix_Chunk*> cache_map;
cache_map cache;

Mix_Music* current_music = NULL;
std::string current_music_name;
std::string next_music;

//function which gets called when music finishes playing. It starts playing
//of the next scheduled track, if there is one.
void on_music_finished()
{
	Mix_FreeMusic(current_music);
	current_music = NULL;
	if(next_music.empty() == false) {
		play_music(next_music);
	}
	next_music.clear();
}

}

manager::manager()
{
	if(preferences::no_sound()) {
		return;
	}

	if(SDL_WasInit(SDL_INIT_AUDIO) == 0) {
		if(SDL_InitSubSystem(SDL_INIT_AUDIO) == -1) {
			sound_ok = false;
			std::cerr << "failed to init sound!\n";
			return;
		}
	}

	if(Mix_OpenAudio(SampleRate, MIX_DEFAULT_FORMAT, 2, BufferSize) == -1) {
		sound_ok = false;
		std::cerr << "failed to open audio!\n";
		return;
	}

	Mix_AllocateChannels(NumChannels);
	sound_ok = true;

	Mix_HookMusicFinished(on_music_finished);
	Mix_VolumeMusic(MIX_MAX_VOLUME);
}

manager::~manager()
{
	if(preferences::no_sound()) {
		return;
	}

	Mix_HookMusicFinished(NULL);
	next_music.clear();
	Mix_CloseAudio();
}

bool ok() { return sound_ok; }
bool muted() { return mute_; }

void mute (bool flag)
{
	mute_ = flag;
	Mix_VolumeMusic(MIX_MAX_VOLUME*(!flag));
}

namespace {

//record which channels sounds are playing on, in case we
//want to cancel a sound.
struct sound_playing {
	std::string file;
	const void* object;
	int		loops;		//not strictly boolean.  -1=true, 0=false
};

std::vector<sound_playing> channels_to_sounds_playing;

int play_internal(const std::string& file, int loops, const void* object)
{
	if(!sound_ok) {
		return -1;
	}

	Mix_Chunk*& chunk = cache[file];
	if(chunk == NULL) {
		chunk = Mix_LoadWAV(("sounds/" + file).c_str());
		if(chunk == NULL) {
			return -1;
		}
	}

	int result = Mix_PlayChannel(-1, chunk, loops);

	//record which channel the sound is playing on.
	if(result >= 0) {
		if(channels_to_sounds_playing.size() <= result) {
			channels_to_sounds_playing.resize(result + 1);
		}

		channels_to_sounds_playing[result].file = file;
		channels_to_sounds_playing[result].object = object;
		channels_to_sounds_playing[result].loops = loops;
	}

	return result;
}

}

void play(const std::string& file, const void* object)
{
	if(preferences::no_sound() || mute_) {
		return;
	}

	play_internal(file, 0, object);
}

void stop_sound(const std::string& file, const void* object)
{
	for(int n = 0; n != channels_to_sounds_playing.size(); ++n) {
		if(channels_to_sounds_playing[n].object == object &&
		   channels_to_sounds_playing[n].file == file) {
			Mix_HaltChannel(n);
		}
	}
}
	
void stop_looped_sounds(const void* object)
{
	for(int n = 0; n != channels_to_sounds_playing.size(); ++n) {
		if(channels_to_sounds_playing[n].object == object &&
		   (channels_to_sounds_playing[n].loops != 0)) {
			fprintf(stderr, "HALTING SOUND: %s, LOOP VAlUE=%d, OBJECT=%p\n", channels_to_sounds_playing[n].file.c_str(),channels_to_sounds_playing[n].loops, channels_to_sounds_playing[n].object); 
			Mix_HaltChannel(n);
		} else if(channels_to_sounds_playing[n].object == object) {
			//this sound is a looped sound, but make sure it keeps going
			//until it ends, since this function signals that the associated
			//object is going away.
			channels_to_sounds_playing[n].object = NULL;
		}
	}
}
	
int play_looped(const std::string& file, const void* object)
{
	if(preferences::no_sound() || mute_) {
		return -1;
	}


	const int result = play_internal(file, -1, object);
	std::cerr << "PLAY: " << object << " " << file << " -> " << result << "\n";
	return result;
}

void cancel_looped(int handle)
{
	Mix_HaltChannel(handle);
}

void play_music(const std::string& file)
{
	if(preferences::no_sound() || !sound_ok) {
		return;
	}

	if(file.empty() || file == current_music_name) {
		return;
	}

	if(current_music) {
		next_music = file;
		Mix_FadeOutMusic(1000);
		return;
	}

	current_music_name = file;
	current_music = Mix_LoadMUS(("music/" + file).c_str());
	if(!current_music) {
		std::cerr << "Mix_LoadMUS ERROR loading " << file << ": " << Mix_GetError() << "\n";
		return;
	}

	Mix_FadeInMusic(current_music, -1, 1000);
}

void play_music_interrupt(const std::string& file)
{
	if(preferences::no_sound()) {
		return;
	}

	if(next_music.empty() == false) {
		current_music_name = next_music;
		next_music.clear();
	}

	Mix_HaltMusic();

	next_music = current_music_name;
	
	current_music_name = file;
	current_music = Mix_LoadMUS(("music/" + file).c_str());
	if(!current_music) {
		std::cerr << "Mix_LoadMUS ERROR loading " << file << ": " << Mix_GetError() << "\n";
		return;
	}

	Mix_PlayMusic(current_music, 1);
}

}
