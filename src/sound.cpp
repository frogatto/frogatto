#include <iostream>
#include <map>
#include <vector>

#include <boost/shared_ptr.hpp>

#include "preferences.hpp"
#include "SDL.h"
#include "SDL_mixer.h"

#include "sound.hpp"

#if TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE
#include "iphone_sound.h"
#endif

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
std::string current_music_name;
std::string next_music;
#if !TARGET_IPHONE_SIMULATOR && !TARGET_OS_IPHONE
Mix_Music* current_music = NULL;
#endif

//function which gets called when music finishes playing. It starts playing
//of the next scheduled track, if there is one.
void on_music_finished()
{
#if !TARGET_IPHONE_SIMULATOR && !TARGET_OS_IPHONE
	Mix_FreeMusic(current_music);
	current_music = NULL;
#endif
	if(next_music.empty() == false) {
		play_music(next_music);
	}
	next_music.clear();
}

#if TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE
	
struct
{
	/* channel array holds information about currently playing sounds */
	struct
	{
		Uint8 *position; /* what is the current position in the buffer of this sound ? */
		Uint32 remaining; /* how many bytes remaining before we're done playing the sound ? */
		Uint32 timestamp; /* when did this sound start playing ? */
	} channels[NumChannels];
	SDL_AudioSpec outputSpec; /* what audio format are we using for output? */
	int numSoundsPlaying; /* how many sounds are currently playing */
} mixer;

class sound
{
	public:
	boost::shared_ptr<Uint8> buffer; /* audio buffer for sound file */
	Uint32 length; /* length of the buffer (in bytes) */
	sound (const std::string& file = "") : length(0)
	{
		SDL_AudioSpec spec; /* the audio format of the .wav file */
		SDL_AudioCVT cvt; /* used to convert .wav to output format when formats differ */
		Uint8 *tmp_buffer;
		if (SDL_LoadWAV(file.c_str(), &spec, &tmp_buffer, &length) == NULL)
		{
			std::cerr << "Could not load sound: " << file << "\n";
			return; //should maybe die
		}
		/* build the audio converter */
		int result = SDL_BuildAudioCVT(&cvt, spec.format, spec.channels, spec.freq,
			mixer.outputSpec.format, mixer.outputSpec.channels, mixer.outputSpec.freq);
		if (result == -1)
		{
			std::cerr << "Could not build audio CVT for: " << file << "\n";
			return; //should maybe die
		} else if (result != 0) {
			/* 
			 this happens when the .wav format differs from the output format.
			 we convert the .wav buffer here
			 */
			cvt.buf = (Uint8 *) SDL_malloc(length * cvt.len_mult); /* allocate conversion buffer */
			cvt.len = length; /* set conversion buffer length */
			SDL_memcpy(cvt.buf, tmp_buffer, length); /* copy sound to conversion buffer */
			if (SDL_ConvertAudio(&cvt) == -1) /* convert the sound */
			{
				std::cerr << "Could not convert sound: " << file << "\n";
				return; //should maybe die
			}
			SDL_free(tmp_buffer); /* free the original (unconverted) buffer */
			tmp_buffer = cvt.buf; /* point sound buffer to converted buffer */
			length = cvt.len_cvt; /* set sound buffer's new length */
		}
		buffer = boost::shared_ptr<Uint8>(tmp_buffer);
	}
	
	bool operator==(void *p) {return buffer.get() == p;}
};
	
void sdl_stop_channel (int channel)
{
	mixer.channels[channel].position = NULL;  /* indicates no sound playing on channel anymore */
	mixer.numSoundsPlaying--;
	if (mixer.numSoundsPlaying == 0)
	{
		/* if no sounds left playing, pause audio callback */
		SDL_PauseAudio(1);
	}
}

void sdl_audio_callback (void *userdata, Uint8 * stream, int len)
{
	int i;
	int copy_amt;
	SDL_memset(stream, mixer.outputSpec.silence, len);  /* initialize buffer to silence */
	/* for each channel, mix in whatever is playing on that channel */
	for (i = 0; i < NumChannels; i++)
	{
		if (mixer.channels[i].position == NULL)
		{
			/* if no sound is playing on this channel */
			continue;           /* nothing to do for this channel */
		}
		
		/* copy len bytes to the buffer, unless we have fewer than len bytes remaining */
		copy_amt = mixer.channels[i].remaining < len ? mixer.channels[i].remaining : len;
		
		/* mix this sound effect with the output */
		SDL_MixAudioFormat(stream, mixer.channels[i].position, mixer.outputSpec.format, copy_amt, 150);
		
		/* update buffer position in sound effect and the number of bytes left */
		mixer.channels[i].position += copy_amt;
		mixer.channels[i].remaining -= copy_amt;
		
		/* did we finish playing the sound effect ? */
		if (mixer.channels[i].remaining == 0)
		{
			sdl_stop_channel(i);
		}
	}
}

int sdl_play_sound (sound s)
{
	/*
	 find an empty channel to play on.
	 if no channel is available, use oldest channel
	 */
	int i;
	int selected_channel = -1;
	int oldest_channel = 0;
	
	if (mixer.numSoundsPlaying == 0) {
		/* we're playing a sound now, so start audio callback back up */
		SDL_PauseAudio(0);
	}
	
	/* find a sound channel to play the sound on */
	for (i = 0; i < NumChannels; i++) {
		if (mixer.channels[i].position == NULL) {
			/* if no sound on this channel, select it */
			selected_channel = i;
			break;
		}
		/* if this channel's sound is older than the oldest so far, set it to oldest */
		if (mixer.channels[i].timestamp < mixer.channels[oldest_channel].timestamp)
			oldest_channel = i;
	}
	
	/* no empty channels, take the oldest one */
	if (selected_channel == -1)
		selected_channel = oldest_channel;
	else
		mixer.numSoundsPlaying++;
	
	/* point channel data to wav data */
	mixer.channels[selected_channel].position = s.buffer.get();
	mixer.channels[selected_channel].remaining = s.length;
	mixer.channels[selected_channel].timestamp = SDL_GetTicks();
	
	return selected_channel;
}

#endif
	
#if !TARGET_IPHONE_SIMULATOR && !TARGET_OS_IPHONE
typedef std::map<std::string, Mix_Chunk*> cache_map;
#else
typedef std::map<std::string, sound> cache_map;
#endif
cache_map cache;

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

#if !TARGET_IPHONE_SIMULATOR && !TARGET_OS_IPHONE

	if(Mix_OpenAudio(SampleRate, MIX_DEFAULT_FORMAT, 2, BufferSize) == -1) {
		sound_ok = false;
		std::cerr << "failed to open audio!\n";
		return;
	}

	Mix_AllocateChannels(NumChannels);
	sound_ok = true;

	Mix_HookMusicFinished(on_music_finished);
	Mix_VolumeMusic(MIX_MAX_VOLUME);
#else
	iphone_init_music();
	sound_ok = true;
	
	/* initialize the mixer */
	SDL_memset(&mixer, 0, sizeof(mixer));
	/* setup output format */
	mixer.outputSpec.freq = SampleRate;
	mixer.outputSpec.format = AUDIO_S16LSB;
	mixer.outputSpec.channels = 2;
	mixer.outputSpec.samples = 256;
	mixer.outputSpec.callback = sdl_audio_callback;
	mixer.outputSpec.userdata = NULL;
	
	/* open audio for output */
    if (SDL_OpenAudio(&mixer.outputSpec, NULL) != 0)
	{
		std::cerr << "Opening audio failed\n";
		sound_ok = false;
    }
#endif
}

manager::~manager()
{
	if(preferences::no_sound()) {
		return;
	}

#if !TARGET_IPHONE_SIMULATOR && !TARGET_OS_IPHONE
	Mix_HookMusicFinished(NULL);
	next_music.clear();
	Mix_CloseAudio();
#else
	iphone_kill_music();
#endif
}

bool ok() { return sound_ok; }
bool muted() { return mute_; }

void mute (bool flag)
{
	mute_ = flag;
#if !TARGET_IPHONE_SIMULATOR && !TARGET_OS_IPHONE
	Mix_VolumeMusic(MIX_MAX_VOLUME*(!flag));
#endif
}

namespace {

//record which channels sounds are playing on, in case we
//want to cancel a sound.
struct sound_playing {
	std::string file;
	const void* object;
	int	loops;		//not strictly boolean.  -1=true, 0=false
};

std::vector<sound_playing> channels_to_sounds_playing;

int play_internal(const std::string& file, int loops, const void* object)
{
	if(!sound_ok) {
		return -1;
	}

#if !TARGET_IPHONE_SIMULATOR && !TARGET_OS_IPHONE
	Mix_Chunk*& chunk = cache[file];
	if(chunk == NULL) {
		chunk = Mix_LoadWAV(("sounds/" + file).c_str());
		if(chunk == NULL) {
			return -1;
		}
	}

	int result = Mix_PlayChannel(-1, chunk, loops);

#else
	sound& s = cache[file];
	if (s == NULL)
	{
		std::string wav_file = file;
		wav_file.replace(wav_file.length()-3, wav_file.length(), "wav");
		s = sound("sounds/wave/" + wav_file);
		if (s == NULL)
		{
			return -1;
		}
	}
	
	int result = sdl_play_sound(s);
#endif

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
#if !TARGET_IPHONE_SIMULATOR && !TARGET_OS_IPHONE
			Mix_HaltChannel(n);
#else
			sdl_stop_channel(n);
#endif
		}
	}
}
	
void stop_looped_sounds(const void* object)
{
#if !TARGET_IPHONE_SIMULATOR && !TARGET_OS_IPHONE
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
#endif
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

void change_volume(const void* object, int volume)
{
	//Note - range is 0-128 (MIX_MAX_VOLUME).  Truncate:
	if( volume > 128){
		volume = 128;
	} else if ( volume < 0 ){
		volume = 0;
	}
	
#if !TARGET_IPHONE_SIMULATOR && !TARGET_OS_IPHONE
	//find the channel associated with this object.
	for(int n = 0; n != channels_to_sounds_playing.size(); ++n) {
		if(channels_to_sounds_playing[n].object == object) {
			Mix_Volume(n, volume);
		} //else, we just do nothing
	}
#endif
}
	
void cancel_looped(int handle)
{
#if !TARGET_IPHONE_SIMULATOR && !TARGET_OS_IPHONE
	Mix_HaltChannel(handle);
#endif
}

void play_music(const std::string& file)
{
	if(preferences::no_sound() || !sound_ok) {
		return;
	}

	if(file.empty() || file == current_music_name) {
		return;
	}

#if !TARGET_IPHONE_SIMULATOR && !TARGET_OS_IPHONE
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
#else
	std::string aac_file = file;
	aac_file.replace(aac_file.length()-3, aac_file.length(), "m4a");
	iphone_play_music(("music_aac/" + aac_file).c_str());
	current_music_name = file;
#endif
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
	
	next_music = current_music_name;
	
	current_music_name = file;

#if !TARGET_IPHONE_SIMULATOR && !TARGET_OS_IPHONE
	Mix_HaltMusic();
	current_music = Mix_LoadMUS(("music/" + file).c_str());
	if(!current_music) {
		std::cerr << "Mix_LoadMUS ERROR loading " << file << ": " << Mix_GetError() << "\n";
		return;
	}

	Mix_PlayMusic(current_music, 1);
#else
	std::string aac_file = file;
	aac_file.replace(aac_file.length()-3, aac_file.length(), "m4a");
	iphone_play_music(("music_aac/" + aac_file).c_str());
#endif
}

}
