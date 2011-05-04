#import "SampleOFDelegate.h"
#import "OpenFeint+UserOptions.h"

#include "globals.h"

@implementation SampleOFDelegate

- (void)dashboardWillAppear
{
	pause_stack++;
	of_dashboard_visible = true;
}

- (void)dashboardDidAppear
{
}

- (void)dashboardWillDisappear
{
}

- (void)dashboardDidDisappear
{
	pause_stack--;
	of_dashboard_visible = false;
}

- (void)offlineUserLoggedIn:(NSString*)userId
{
	NSLog(@"User logged in, but OFFLINE. UserId: %@", userId);
}

- (void)userLoggedIn:(NSString*)userId
{
	NSLog(@"User logged in. UserId: %@", userId);
}

- (BOOL)showCustomOpenFeintApprovalScreen
{
	return NO;
}

@end