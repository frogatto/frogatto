#ifndef PREFERENCES_HPP_INCLUDED
#define PREFERENCES_HPP_INCLUDED

namespace preferences {

bool parse_arg(const char* arg);
bool no_music();
bool show_debug_hitboxes();
bool use_pretty_scaling();
void set_use_pretty_scaling(bool value);
}

#endif
