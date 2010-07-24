#ifndef IPHONE_SOUND_H_INCLUDED
#define IPHONE_SOUND_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

void iphone_init_music (void (*callback)());
void iphone_fade_in_music (int duration);
void iphone_fade_out_music (int duration);
void iphone_play_music (const char *file, int loops);
void iphone_pause_music ();
void iphone_resume_music();
void iphone_kill_music ();
float iphone_get_music_volume ();
void iphone_set_music_volume (float v);

#ifdef __cplusplus
} //extern "C"
#endif

#endif