#ifndef IPHONE_SOUND_H_INCLUDED
#define IPHONE_SOUND_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

void iphone_init_music ();
void iphone_play_music (const char *file);
void iphone_pause_music ();
void iphone_resume_music();
void iphone_kill_music ();

#ifdef __cplusplus
} //extern "C"
#endif

#endif