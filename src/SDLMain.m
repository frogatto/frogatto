/*   SDLMain.m - main entry point for our Cocoa-ized SDL app
       Initial Version: Darrell Walisser <dwaliss1@purdue.edu>
       Non-NIB-Code & other changes: Max Horn <max@quendi.de>

    Feel free to customize this file to suit your needs

	Heavily modified by Ben Anderman.
*/

#import "SDL.h"
#import "SDLMain.h"
#import <sys/param.h> /* for MAXPATHLEN */
#import <unistd.h>

#include "utils.hpp"

static int    gArgc;
static char  **gArgv;
static BOOL   gFinderLaunch;

@interface SDLApplication : NSApplication
@end

@implementation SDLApplication
/* Invoked from the Quit menu item */
- (void)terminate:(id)sender
{
    /* Post a SDL_QUIT event */
    SDL_Event event;
    event.type = SDL_QUIT;
    SDL_PushEvent(&event);
}

- (BOOL)_handleKeyEquivalent:(NSEvent *)theEvent
{
	[[self mainMenu] performKeyEquivalent:theEvent];
	return YES;
}

- (void) sendEvent:(NSEvent *)event
{
	if(NSKeyDown == [event type] || NSKeyUp == [event type])
	{
		if([event modifierFlags] & NSCommandKeyMask) 
			[super sendEvent: event];
	} else {
		[super sendEvent: event];
	}
}
@end


/* The main class of the application, the application's delegate */
@implementation SDLMain

/* Called when the internal event loop has just started running */
- (void) applicationDidFinishLaunching: (NSNotification *) note
{
	setenv ("SDL_ENABLEAPPEVENTS", "1", 1);
    int status;

    /* Set the working directory to the .app's parent directory */
    chdir([[[NSBundle mainBundle] resourcePath] fileSystemRepresentation]);

    /* Hand off to main application code */
    status = SDL_main (gArgc, gArgv);

    /* We're done, thank you for playing */
    exit(status);
}

- (IBAction) toggleFullscreen: (id)sender
{
	toggle_fullscreen();
}

@end

#ifdef main
#  undef main
#endif

/* Main entry point to executable - should *not* be SDL_main! */
int main (int argc, char **argv)
{
    /* Copy the arguments into a global variable */
    int i;
    
    /* This is passed if we are launched by double-clicking */
    if ( argc >= 2 && strncmp (argv[1], "-psn", 4) == 0 ) {
        gArgc = 1;
	gFinderLaunch = YES;
    } else {
        gArgc = argc;
	gFinderLaunch = NO;
    }
    gArgv = (char**) malloc (sizeof(*gArgv) * (gArgc+1));
    assert (gArgv != NULL);
    for (i = 0; i < gArgc; i++)
        gArgv[i] = argv[i];
    gArgv[i] = NULL;

	[SDLApplication sharedApplication];
    [NSBundle loadNibNamed:@"SDLMain" owner:NSApp];
    [NSApp run];
    return 0;
}
