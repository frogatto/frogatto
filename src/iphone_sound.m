#import "iphone_sound.h"
#include "SDL.h"

#import <AVFoundation/AVFoundation.h>
#import <AudioToolbox/AudioToolbox.h>

static AVAudioPlayer *song = nil;
static const float fade_interval = 0.05;
static float master_volume = 1.0;
static void (*song_finished_callback)() = NULL;

@interface AudioDelegate : NSObject <AVAudioPlayerDelegate>
{
	float amount;
	BOOL fading;
}

@property (readwrite) float amount;
@property (readonly) BOOL fading;

- (void) fadeInMusic:(id)obj;
- (void) fadeOutMusic:(id)obj;

- (void) audioPlayerDidFinishPlaying:(AVAudioPlayer *)player successfully:(BOOL)flag;
@end

AudioDelegate *music_delegate;

void iphone_init_music (void (*callback)())
{
	music_delegate = [[AudioDelegate alloc] init];
	song_finished_callback = callback;
	
	AudioSessionInitialize (NULL, NULL, NULL, NULL);
	AudioSessionSetActive (true);
	UInt32 sessionCategory = kAudioSessionCategory_SoloAmbientSound;
	AudioSessionSetProperty (kAudioSessionProperty_AudioCategory, sizeof(sessionCategory), &sessionCategory);
}

void iphone_fade_in_music (int duration)
{
	if (song == nil || music_delegate == nil || music_delegate.fading) return;
	song.volume = 0.0;
	music_delegate.amount = master_volume/((duration/1000.0) * (1.0/fade_interval));
	//NSLog(@"Starting fade in, volume=%f | amount=%f", song.volume, music_delegate.amount);
	[music_delegate fadeInMusic:nil];
}

void iphone_fade_out_music (int duration)
{
	if (song == nil || music_delegate == nil || music_delegate.fading) return;
	music_delegate.amount = master_volume/((duration/1000.0) * (1.0/fade_interval));
	[music_delegate fadeOutMusic:nil];
}

void iphone_play_music (const char *file, int loops)
{
	int timer = SDL_GetTicks();
	if (song)
	{
		[song stop];
		[song release];
	}
	song = [[AVAudioPlayer alloc] initWithContentsOfURL:[NSURL fileURLWithPath:[NSString stringWithUTF8String: file]] error:NULL];
	//NSLog(@"Initializing the new song took %i ms", SDL_GetTicks()-timer);
	song.delegate = music_delegate;
	song.numberOfLoops = loops;
	song.volume = master_volume;
	
	timer = SDL_GetTicks();
	[song play];
	//NSLog(@"Playing the new song (%s) took %i ms", file, SDL_GetTicks()-timer);
}

void iphone_pause_music ()
{
	[song pause];
}

void iphone_resume_music ()
{
	[song play];
}

void iphone_kill_music ()
{
	if (song)
	{
		[song stop];
		[song release];
		song = nil;
	}
	[music_delegate release];
	music_delegate = nil;
}

float iphone_get_music_volume ()
{
	return master_volume;
}

void iphone_set_music_volume (float v)
{
	master_volume = v;
	if (song != nil)
		song.volume = master_volume;
}

@implementation AudioDelegate

@synthesize amount;
@synthesize fading;

- (void) fadeInMusic:(id)obj
{
	if (song.volume + amount >= master_volume)
	{
		//NSLog(@"Done fading in.");
		fading = NO;
		song.volume = master_volume; //fading is done
	} else {
		fading = YES;
		song.volume += amount;
		//NSLog(@"Fading in, volume=%f", song.volume);
		[self performSelector:@selector(fadeInMusic:) withObject:nil afterDelay:fade_interval];
	}
}

- (void) fadeOutMusic:(id)obj
{
	if (song.volume - amount <= 0.0)
	{
		fading = NO;
		song.volume = 0.0; //fading is done, and the volume should be exactly 0.0
		[self audioPlayerDidFinishPlaying:song successfully:YES];
	} else {
		fading = YES;
		song.volume -= amount;
		[self performSelector:@selector(fadeOutMusic:) withObject:nil afterDelay:fade_interval];
	}
}

- (void) audioPlayerDidFinishPlaying:(AVAudioPlayer *)player successfully:(BOOL)flag
{
	[player stop];
	[player release];
	song = nil;
	(*song_finished_callback)();
	NSLog(@"Finished playing music");
}

- (void) audioPlayerEndInterruption:(AVAudioPlayer *)player
{
	[player play];
}

@end
