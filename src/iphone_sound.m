#import "iphone_sound.h"
#import "GBMusicTrack.h"

//#import <AVFoundation/AVFoundation.h>

GBMusicTrack *song = nil;
//AVAudioPlayer *avPlayer = nil;

void iphone_init_music ()
{
}

void iphone_play_music (const char *file)
{
	if (song)
	{
		[song close];
	}
	song = [[GBMusicTrack alloc] initWithPath:[NSString stringWithCString: file]];
	[song setRepeat:YES];
	[song play];
//	if (!avPlayer)
//	{
//		avPlayer = [[AVAudioPlayer alloc] initWithContentsOfURL:[NSURL URLWithString:[NSString stringWithCString: file]] error:NULL];
//		[avPlayer play];
//	}
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
		[song close];
		[song release];
		song = nil;
	}
}
