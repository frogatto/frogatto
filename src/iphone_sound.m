#import "iphone_sound.h"
#import "GBMusicTrack.h"

#import <AVFoundation/AVFoundation.h>

//GBMusicTrack *song = nil;
AVAudioPlayer *song = nil;
const float fade_interval = 0.05;
void (*song_finished_callback)() = NULL;

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

AudioDelegate *delegate;

void iphone_init_music (void (*callback)())
{
	delegate = [[AudioDelegate alloc] init];
	song_finished_callback = callback;
}

void iphone_fade_in_music (int duration)
{
	if (song == nil || delegate == nil || delegate.fading) return;
	song.volume = 0.0;
	delegate.amount = 1.0/((duration/1000.0) * (1.0/fade_interval));
	//NSLog(@"Starting fade in, volume=%f | amount=%f", song.volume, delegate.amount);
	[delegate fadeInMusic:nil];
}

void iphone_fade_out_music (int duration)
{
	if (song == nil || delegate == nil || delegate.fading) return;
	delegate.amount = 1.0/((duration/1000.0) * (1.0/fade_interval));
	[delegate fadeOutMusic:nil];
}

void iphone_play_music (const char *file)
{
//	if (song)
//	{
//		[song close];
//	}
//	song = [[GBMusicTrack alloc] initWithPath:[NSString stringWithCString: file]];
//	[song setRepeat:YES];
//	[song play];
	if (song)
	{
		[song stop];
		[song release];
	}
	song = [[AVAudioPlayer alloc] initWithContentsOfURL:[NSURL URLWithString:[NSString stringWithCString: file]] error:NULL];
	song.delegate = delegate;
	[song play];
}

void iphone_pause_music ()
{
	if (song)
	{
		[song pause];
	}
}

void iphone_resume_music ()
{
	if (song)
	{
		[song play];
	}
}

void iphone_kill_music ()
{
	if (song)
	{
		[song stop];
		[song release];
		song = nil;
	}
	[delegate release];
	delegate = nil;
}

@implementation AudioDelegate

@synthesize amount;
@synthesize fading;

- (void) fadeInMusic:(id)obj
{
	if (song.volume + amount >= 1.0)
	{
		//NSLog(@"Done fading in.");
		fading = NO;
		song.volume = 1.0; //fading is done, and the volume should be exactly 1.0
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
}

@end
