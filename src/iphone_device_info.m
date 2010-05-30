#import <UIKit/UIKit.h>

#import "iphone_device_info.h"

void iphone_screen_res (int *w, int *h)
{
	UIScreen * screen = [UIScreen mainScreen];
	if ([UIScreen instancesRespondToSelector:@selector(currentMode)])
	{
		UIScreenMode * mode = [screen currentMode];
		CGSize size = [mode size];
		*w = size.width;
		*h = size.height;
	} else {
		*w = 320;
		*h = 480;
	}
}
