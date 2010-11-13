#include "of_bridge.h"

#ifdef ENABLE_OPENFEINT

#include "OpenFeint.h"
#include "OFAchievementService.h"
#include "OFAchievement.h"
#include "SampleOFDelegate.h"
#include "Appirater.h"

static SampleOFDelegate* delegate;

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
	
	[Appirater appLaunched];
}

void of_dashboard ()
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	[OpenFeint launchDashboard];
	[pool release];
}

void of_earn_achievement (int of_id)
{
	[[OFAchievement achievement: [NSString stringWithFormat:@"%d", of_id]] updateProgressionComplete: 100.0f andShowNotification: NO];
}

#else

//Dummy functions for if OpenFeint isn't enabled

void of_init () {}
void of_dashboard () {}
void of_earn_achievement (int of_id) {}

#endif