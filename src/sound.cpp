#include <iostream>
#include <map>
#include <vector>

#include <pthread.h>

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>

#include "foreach.hpp"
#include "preferences.hpp"
#include "thread.hpp"
#include "filesystem.hpp"
#include "graphics.hpp"

#include "sound.hpp"

#if TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE
#include "iphone_sound.h"
#endif

#include "wml_node.hpp"
#include "wml_utils.hpp"

namespace sound {

namespace {

struct MusicInfo {
	MusicInfo() : volume(1.0) {}
	float volume;
};

std::map<std::string, MusicInfo> music_index;

float sfx_volume = 1.0;
float user_music_volume = 1.0;
float engine_music_volume = 1.0;
const int SampleRate = 44100;
// number of allocated channels, 
#if !TARGET_IPHONE_SIMULATOR && !TARGET_OS_IPHONE
const size_t NumChannels = 16;
#else
const size_t NumChannels = 16;
#endif

#ifdef WIN32
const size_t BufferSize = 4096;
#else
const size_t BufferSize = 1024;
#endif

bool sound_ok = false;
bool mute_ = false;
std::string& current_music_name() {
	static std::string name;
	return name;
}

std::string& next_music() {
	static std::string name;
	return name;
}

#if !TARGET_IPHONE_SIMULATOR && !TARGET_OS_IPHONE
Mix_Music* current_mix_music = NULL;
#else
bool playing_music = false;
#endif

//function which gets called when music finishes playing. It starts playing
//of the next scheduled track, if there is one.
void on_music_finished()
{
#if !TARGET_IPHONE_SIMULATOR && !TARGET_OS_IPHONE
	Mix_FreeMusic(current_mix_music);
	current_mix_music = NULL;
#else
	playing_music = false;
#endif
	if(next_music().empty() == false) {
		play_music(next_music());
	}
	next_music().clear();
}

//record which channels sounds are playing on, in case we
//want to cancel a sound.
struct sound_playing {
	std::string file;
	const void* object;
	int	loops;		//not strictly boolean.  -1=true, 0=false
	float volume;
};

std::vector<sound_playing> channels_to_sounds_playing, queued_sounds;

void on_sound_finished(int channel)
{
	if(channel >= 0 && channel < channels_to_sounds_playing.size()) {
		channels_to_sounds_playing[channel].object = NULL;
	}
}

#if TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE

class sound; //so mixer can make pointers to it

struct mixer
{
	/* channel array holds information about currently playing sounds */
	struct
	{
		Uint8 *position; /* what is the current position in the buffer of this sound ? */
		Uint32 remaining; /* how many bytes remaining before we're done playing the sound ? */
		Uint32 timestamp; /* when did this sound start playing ? */
		int volume;
		int loops;
		sound *s;
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
		if (file == "") return;
		SDL_AudioSpec spec; /* the audio format of the .wav file */
		SDL_AudioCVT cvt; /* used to convert .wav to output format when formats differ */
		Uint8 *tmp_buffer;
		if (SDL_LoadWAV(file.c_str(), &spec, &tmp_buffer, &length) == NULL)
		{
			std::cerr << "Could not load sound: " << file << "\n";
			return; //should maybe die
		}
		buffer = boost::shared_ptr<Uint8>(tmp_buffer, SDL_free);
		return; // don't convert the audio, assume it's already in the right format
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
			SDL_memcpy(cvt.buf, buffer.get(), length); /* copy sound to conversion buffer */
			if (SDL_ConvertAudio(&cvt) == -1) /* convert the sound */
			{
				std::cerr << "Could not convert sound: " << file << "\n";
				SDL_free(cvt.buf);
				return; //should maybe die
			}
			buffer = boost::shared_ptr<Uint8>(cvt.buf, SDL_free); /* point sound buffer to converted buffer */
			length = cvt.len_cvt; /* set sound buffer's new length */
		}
	}
	
	bool operator==(void *p) {return buffer.get() == p;}
};
	
void sdl_stop_channel (int channel)
{
	if (mixer.channels[channel].position == NULL) return; // if the sound was playing in the first place
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
		SDL_MixAudioFormat(stream, mixer.channels[i].position, mixer.outputSpec.format, copy_amt, sfx_volume * mixer.channels[i].volume);
		
		/* update buffer position in sound effect and the number of bytes left */
		mixer.channels[i].position += copy_amt;
		mixer.channels[i].remaining -= copy_amt;
		
		/* did we finish playing the sound effect ? */
		if (mixer.channels[i].remaining == 0)
		{
			if (mixer.channels[i].loops != 0)
			{
				mixer.channels[i].position = mixer.channels[i].s->buffer.get();
				mixer.channels[i].remaining = mixer.channels[i].s->length;
				if (mixer.channels[i].loops != -1) mixer.channels[i].loops--;
			} else {
				sdl_stop_channel(i);
			}
		}
	}
}

int sdl_play_sound (sound *s, int loops)
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
	mixer.channels[selected_channel].position = s->buffer.get();
	mixer.channels[selected_channel].remaining = s->length;
	mixer.channels[selected_channel].timestamp = SDL_GetTicks();
	mixer.channels[selected_channel].volume = SDL_MIX_MAXVOLUME;
	mixer.channels[selected_channel].loops = loops;
	mixer.channels[selected_channel].s = s;
	
	return selected_channel;
}

#endif
	
#if !TARGET_IPHONE_SIMULATOR && !TARGET_OS_IPHONE
typedef std::map<std::string, Mix_Chunk*> cache_map;
#else
typedef std::map<std::string, sound> cache_map;
#endif
cache_map cache;

cache_map threaded_cache;
threading::mutex cache_mutex;


bool sound_init = false;

void thread_load(const std::string& file)
{
#if !TARGET_IPHONE_SIMULATOR && !TARGET_OS_IPHONE
	Mix_Chunk* chunk = Mix_LoadWAV(("sounds/" + file).c_str());

	{
		threading::lock l(cache_mutex);
		threaded_cache[file] = chunk;
	}

#else
	std::string wav_file = file;
	wav_file.replace(wav_file.length()-3, wav_file.length(), "wav");
	sound s("sounds_wav/" + wav_file);

	{
		threading::lock l(cache_mutex);
		threaded_cache[file] = s;
	}
#endif
}

std::map<std::string, boost::shared_ptr<threading::thread> > loading_threads;

}

manager::manager()
{
	sound_init = true;
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

	Mix_ChannelFinished(on_sound_finished);
	Mix_HookMusicFinished(on_music_finished);
	Mix_VolumeMusic(MIX_MAX_VOLUME);
#else
	iphone_init_music(on_music_finished);
	sound_ok = true;
	
	/* initialize the mixer */
	SDL_memset(&mixer, 0, sizeof(mixer));
	/* setup output format */
	mixer.outputSpec.freq = SampleRate;
	mixer.outputSpec.format = AUDIO_S16LSB;
	mixer.outputSpec.channels = 1;
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

	set_music_volume(user_music_volume);
}

manager::~manager()
{
	if(preferences::no_sound()) {
		return;
	}

	loading_threads.clear();

#if !TARGET_IPHONE_SIMULATOR && !TARGET_OS_IPHONE
	Mix_HookMusicFinished(NULL);
	next_music().clear();
	Mix_CloseAudio();
#else
	iphone_kill_music();
#endif
}

void init_music(wml::const_node_ptr node)
{
	FOREACH_WML_CHILD(music_node, node, "music") {
		const std::string name = music_node->attr("name").str();
		music_index[name].volume = wml::get_attr<float>(music_node, "volume", 1.0);
	}
}

bool ok() { return sound_ok; }
bool muted() { return mute_; }

void mute (bool flag)
{
	mute_ = flag;
#if !TARGET_IPHONE_SIMULATOR && !TARGET_OS_IPHONE
	Mix_VolumeMusic(engine_music_volume*MIX_MAX_VOLUME*(!flag));
#endif
}

void preload(const std::string& file)
{
	if(!sound_ok) {
		return;
	}

	if(loading_threads.count(file)) {
		return;
	}

	boost::shared_ptr<threading::thread> t(new threading::thread(boost::bind(thread_load, file)));
	loading_threads[file] = t;
}

namespace {

int play_internal(const std::string& file, int loops, const void* object, float volume)
{
	if(!sound_ok) {
		return -1;
	}

	if(!cache.count(file)) {
		preload(file);
		queued_sounds.push_back(sound_playing());
		queued_sounds.back().file = file;
		queued_sounds.back().loops = loops;
		queued_sounds.back().object = object;
		queued_sounds.back().volume = volume;
		
		return -1;
	}

#if !TARGET_IPHONE_SIMULATOR && !TARGET_OS_IPHONE
	Mix_Chunk* chunk = cache[file];
	if(chunk == NULL) {
		return -1;
	}

	int result = Mix_PlayChannel(-1, chunk, loops);

#else
	sound& s = cache[file];
	if(s == NULL) {
		return -1;
	}
	
	int result = sdl_play_sound(&s, loops);
#endif

	//record which channel the sound is playing on.
	if(result >= 0) {
		if(channels_to_sounds_playing.size() <= result) {
			channels_to_sounds_playing.resize(result + 1);
		}
#if !TARGET_IPHONE_SIMULATOR && !TARGET_OS_IPHONE
		Mix_Volume(result, volume*sfx_volume*MIX_MAX_VOLUME); //start sound at full volume
#endif

		channels_to_sounds_playing[result].file = file;
		channels_to_sounds_playing[result].object = object;
		channels_to_sounds_playing[result].loops = loops;
	}

	return result;
}

}

void process()
{
	bool has_items = false;
	{
		threading::lock l(cache_mutex);
		for(cache_map::const_iterator i = threaded_cache.begin(); i != threaded_cache.end(); ++i) {
			cache.insert(*i);
			has_items = true;
			loading_threads.erase(i->first);
		}

		threaded_cache.clear();
	}

	if(has_items) {
		std::vector<sound_playing> sounds;
		sounds.swap(queued_sounds);
		foreach(const sound_playing& sfx, sounds) {
			play_internal(sfx.file, sfx.loops, sfx.object, sfx.volume);
		}
	}
}

void play(const std::string& file, const void* object, float volume)
{
	if(preferences::no_sound() || mute_) {
		return;
	}

	play_internal(file, 0, object, volume);
}

void stop_sound(const std::string& file, const void* object)
{
	for(int n = 0; n != channels_to_sounds_playing.size(); ++n) {
		if(channels_to_sounds_playing[n].object == object &&
		   channels_to_sounds_playing[n].file == file) {
			channels_to_sounds_playing[n].object = NULL;
#if !TARGET_IPHONE_SIMULATOR && !TARGET_OS_IPHONE
			Mix_HaltChannel(n);
#else
			sdl_stop_channel(n);
#endif
		}
	}

	for(int n = 0; n != queued_sounds.size(); ++n) {
		if(queued_sounds[n].object == object &&
		   queued_sounds[n].file == file) {
			queued_sounds.erase(queued_sounds.begin() + n);
			--n;
		}
	}
}
	
void stop_looped_sounds(const void* object)
{
	for(int n = 0; n != channels_to_sounds_playing.size(); ++n) {
		if((object == NULL && channels_to_sounds_playing[n].object != NULL
		   || channels_to_sounds_playing[n].object == object) &&
		   (channels_to_sounds_playing[n].loops != 0)) {
#if !TARGET_IPHONE_SIMULATOR && !TARGET_OS_IPHONE
			Mix_HaltChannel(n);
#else
			sdl_stop_channel(n);
#endif
			channels_to_sounds_playing[n].object = NULL;
		} else if(channels_to_sounds_playing[n].object == object) {
			//this sound is a looped sound, but make sure it keeps going
			//until it ends, since this function signals that the associated
			//object is going away.
			channels_to_sounds_playing[n].object = NULL;
		}
	}

	for(int n = 0; n != queued_sounds.size(); ++n) {
		if((object == NULL && queued_sounds[n].object != NULL
		   || queued_sounds[n].object == object) &&
		   (queued_sounds[n].loops != 0)) {
			queued_sounds.erase(queued_sounds.begin() + n);
			--n;
		} else if(queued_sounds[n].object == object) {
			//this sound is a looped sound, but make sure it keeps going
			//until it ends, since this function signals that the associated
			//object is going away.
			queued_sounds[n].object = NULL;
		}
	}
}
	
int play_looped(const std::string& file, const void* object, float volume)
{
	if(preferences::no_sound() || mute_) {
		return -1;
	}


	const int result = play_internal(file, -1, object, volume);
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
	
	//find the channel associated with this object.
	for(int n = 0; n != channels_to_sounds_playing.size(); ++n) {
		if(channels_to_sounds_playing[n].object == object) {
#if !TARGET_IPHONE_SIMULATOR && !TARGET_OS_IPHONE
			Mix_Volume(n, sfx_volume*volume);
#else
			mixer.channels[n].volume = sfx_volume*volume;
#endif
		} //else, we just do nothing
	}
}

float get_sound_volume()
{
	return sfx_volume;
}

void set_sound_volume(float volume)
{
	sfx_volume = volume;
}

float get_music_volume()
{
	return user_music_volume;
}

namespace {
void update_music_volume()
{
	if(sound_init) {
#if !TARGET_IPHONE_SIMULATOR && !TARGET_OS_IPHONE
		Mix_VolumeMusic(user_music_volume*engine_music_volume*MIX_MAX_VOLUME);
#else
		iphone_set_music_volume(user_music_volume*engine_music_volume);
#endif
	}
}
}

void set_music_volume(float volume)
{
	user_music_volume = volume;
	update_music_volume();
}

void set_engine_music_volume(float volume)
{
	engine_music_volume = volume;
	update_music_volume();
}

float get_engine_music_volume()
{
	return engine_music_volume;
}

void play_music(const std::string& file)
{
	if(preferences::no_sound() || preferences::no_music() || !sound_ok) {
		return;
	}

	if(file == current_music_name()) {
		return;
	}

#if !TARGET_IPHONE_SIMULATOR && !TARGET_OS_IPHONE
	if(current_mix_music) {
		next_music() = file;
		Mix_FadeOutMusic(500);
		return;
	}

	if(file.empty()) {
		return;
	}

	current_music_name() = file;
	current_mix_music = Mix_LoadMUS(("music/" + file).c_str());
	if(!current_mix_music) {
		std::cerr << "Mix_LoadMUS ERROR loading " << file << ": " << Mix_GetError() << "\n";
		return;
	}

	set_engine_music_volume(music_index[file].volume);

	Mix_FadeInMusic(current_mix_music, -1, 500);
#else
	if (playing_music)
	{
		next_music() = file;
		iphone_fade_out_music(350);
		return;
	}

	if(file.empty()) {
		return;
	}
	
	current_music_name() = file;
	std::string aac_file = file;
	aac_file.replace(aac_file.length()-3, aac_file.length(), "m4a");
	if (!sys::file_exists("music_aac/" + aac_file)) return;
	set_engine_music_volume(music_index[file].volume);
	iphone_play_music(("music_aac/" + aac_file).c_str(), -1);
	iphone_fade_in_music(350);
	playing_music = true;
#endif
}

void play_music_interrupt(const std::string& file)
{
	if(preferences::no_sound() || preferences::no_music()) {
		return;
	}

	if(next_music().empty() == false) {
		current_music_name() = next_music();
		next_music().clear();
	}
	
	next_music() = current_music_name();
	
	current_music_name() = file;

#if !TARGET_IPHONE_SIMULATOR && !TARGET_OS_IPHONE

	//note that calling HaltMusic will result in on_music_finished being
	//called, which releases the current_music pointer.
	Mix_HaltMusic();
	if(file.empty()) {
		return;
	}

	current_mix_music = Mix_LoadMUS(("music/" + file).c_str());
	if(!current_mix_music) {
		std::cerr << "Mix_LoadMUS ERROR loading " << file << ": " << Mix_GetError() << "\n";
		return;
	}

	Mix_PlayMusic(current_mix_music, 1);
#else
	std::string aac_file = file;
	aac_file.replace(aac_file.length()-3, aac_file.length(), "m4a");
	iphone_play_music(("music_aac/" + aac_file).c_str(), 0);
	playing_music = true;
#endif
}

const std::string& current_music() {
	return current_music_name();
}

}
