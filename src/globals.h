#ifndef GLOBALS_H
#define GLOBALS_H

/* This will be incremented when things pop up on the screen which the game should pause for,
 and decremented when those things go away (it's an int in case >1 such things happen at once).
 Perhaps not necessary anymore, since the addition of of_dashboard_visible. */
extern int pause_stack;
extern bool of_dashboard_visible;

#endif
