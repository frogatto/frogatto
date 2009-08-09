#ifndef PREFERENCES_HPP_INCLUDED
#define PREFERENCES_HPP_INCLUDED

namespace preferences {

bool parse_arg(const char* arg);
bool no_sound();
bool show_debug_hitboxes();
bool use_pretty_scaling();
void set_use_pretty_scaling(bool value);
bool fullscreen();
void set_fullscreen(bool value);

void set_widescreen();

int virtual_screen_width();
int virtual_screen_height();

int actual_screen_width();
int actual_screen_height();

void set_actual_screen_width(int width);
void set_actual_screen_height(int height);
}

#endif
