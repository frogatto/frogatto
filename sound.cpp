#include <iostream>
#include <map>

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

typedef std::map<std::string, Mix_Chunk*> cache_map;
cache_map cache;
}

manager::manager()
{
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
}

manager::~manager()
{
	Mix_CloseAudio();
}

bool ok() { return sound_ok; }

void play(const std::string& file)
{
	if(!sound_ok) {
		return;
	}

	Mix_Chunk*& chunk = cache[file];
	if(chunk == NULL) {
		chunk = Mix_LoadWAV(("sounds/" + file).c_str());
		if(chunk == NULL) {
			return;
		}
	}

	Mix_PlayChannel(-1, chunk, 0);
}

}
