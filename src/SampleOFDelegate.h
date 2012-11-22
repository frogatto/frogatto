#pragma once

#import "OpenFeintDelegate.h"

@interface SampleOFDelegate : NSObject< OpenFeintDelegate >

- (void)dashboardWillAppear;
- (void)dashboardDidAppear;
- (void)dashboardWillDisappear;
- (void)dashboardDidDisappear;
- (void)userLoggedIn:(NSString*)userId;
- (BOOL)showCustomOpenFeintApprovalScreen;

@end