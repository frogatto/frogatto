#include "of_bridge.h"
#include "globals.h"

#ifdef ENABLE_OPENFEINT

#include "OpenFeint.h"
#include "OFAchievementService.h"
#include "OFAchievement.h"
#include "OFLeaderboard.h"
#include "SampleOFDelegate.h"
#include "Appirater.h"

static SampleOFDelegate* delegate;

namespace
{
	void of_wait ()
	{
		while (of_dashboard_visible)
		{ 
			CFRunLoopRunInMode(kCFRunLoopDefaultMode, 1, TRUE); 
		}
	}
}

void of_init ()
{
	NSDictionary* settings = [NSDictionary dictionaryWithObjectsAndKeys:
		[NSNumber numberWithInt:UIInterfaceOrientationLandscapeRight], OpenFeintSettingDashboardOrientation,
		@"Frogatto", OpenFeintSettingShortDisplayName, 
		[NSNumber numberWithBool:YES], OpenFeintSettingEnablePushNotifications,
    	nil];
	
	delegate = [SampleOFDelegate new];
	
	OFDelegatesContainer* delegates = [OFDelegatesContainer containerWithOpenFeintDelegate:delegate
		andChallengeDelegate:nil
		andNotificationDelegate:nil];
	
	[OpenFeint initializeWithProductKey:@"0UxncPGMOzy5GVKUW62eqg"
		andSecret: @"WOYWbetXWIFPbRsq6AMjcAn3qcY14KXYmBKSmf6x0"
		andDisplayName: @"Frogatto"
		andSettings: settings
		andDelegates: delegates];
	
	of_wait(); // in case it shows up for the first time to login
	
	[Appirater appLaunched];
}

void of_dashboard ()
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	[OpenFeint launchDashboard];
	of_wait();
	[pool release];
}

void of_earn_achievement (int of_id)
{
	[[OFAchievement achievement: [NSString stringWithFormat:@"%d", of_id]] updateProgressionComplete: 100.0f andShowNotification: NO];
}

void of_submit_score (int of_leaderboard_id, long score)
{
	OFLeaderboard* leaderboard = [OFLeaderboard leaderboard:[NSString stringWithFormat: @"%d", of_leaderboard_id]];
	OFHighScore* hs = [[OFHighScore alloc] initForSubmissionWithScore: score];
	[hs submitTo: leaderboard];
}

#else

//Dummy functions for if OpenFeint isn't enabled

void of_init () {}
void of_dashboard () {}
void of_earn_achievement (int of_id) {}
void of_submit_score (int of_leaderboard_id, long score) {}

#endif