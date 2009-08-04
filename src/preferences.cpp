#include <string>

#include "preferences.hpp"

namespace preferences {
namespace {
bool no_sound_ = false;
bool show_debug_hitboxes_ = false;
bool use_pretty_scaling_ = false;
bool fullscreen_ = false;
}

bool no_sound() {
	return no_sound_;
}

bool show_debug_hitboxes() {
	return show_debug_hitboxes_;
}

bool use_pretty_scaling() {
	return use_pretty_scaling_;
}

void set_use_pretty_scaling(bool value) {
	use_pretty_scaling_ = value;
}

bool fullscreen() {
	return fullscreen_;
}

void set_fullscreen(bool value) {
	fullscreen_ = value;
}

bool parse_arg(const char* arg) {
	std::string s(arg);
	if(s == "--show_hitboxes") {
		show_debug_hitboxes_ = true;
	} else if(s == "--scale") {
		set_use_pretty_scaling(true);
	} else if(s == "--nosound") {
		no_sound_ = true;
	} else if(s == "--fullscreen") {
		fullscreen_ = true;
	} else {
		return false;
	}

	return true;
}
}
