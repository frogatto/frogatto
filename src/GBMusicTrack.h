//
//  GBMusicTrack.h
//  GameBase
//
//  Created by Jake Peterson (AnotherJake) on 7/6/08.
//  Copyright 2008 Jake Peterson. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <AudioToolbox/AudioQueue.h>
#import <AudioToolbox/AudioFile.h>

#define NUM_QUEUE_BUFFERS	3

@interface GBMusicTrack : NSObject
{
	AudioFileID						audioFile;
	AudioStreamBasicDescription		dataFormat;
	AudioQueueRef					queue;
	UInt64							packetIndex;
	UInt32							numPacketsToRead;
	AudioStreamPacketDescription	*packetDescs;
	BOOL							repeat;
	BOOL							trackClosed;
	BOOL							trackEnded;
	AudioQueueBufferRef				buffers[NUM_QUEUE_BUFFERS];
}

- (id)initWithPath:(NSString *)path;
- (void)setGain:(Float32)gain;
- (void)setRepeat:(BOOL)yn;
- (void)play;
- (void)pause;

// close is called automatically in GBMusicTrack's dealloc method, but it is recommended
// to call close first, so that the associated Audio Queue is released immediately, instead
// of having to wait for a possible autorelease, which may cause some conflict
- (void)close;

extern NSString	*GBMusicTrackFinishedPlayingNotification;

@end