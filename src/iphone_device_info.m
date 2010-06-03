#import <UIKit/UIKit.h>

#import "iphone_device_info.h"

void iphone_screen_res (int *w, int *h)
{
	*w = [UIScreen mainScreen].bounds.size.width;
	*h = [UIScreen mainScreen].bounds.size.height;
}
