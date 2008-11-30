#include <string>

#include "preferences.hpp"

namespace preferences {
namespace {
bool no_music_ = false;
bool show_debug_hitboxes_ = false;
bool use_pretty_scaling_ = false;
}

bool no_music() {
	return no_music_;
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

bool parse_arg(const char* arg) {
	std::string s(arg);
	if(s == "--show_hitboxes") {
		show_debug_hitboxes_ = true;
	} else if(s == "--scale") {
		set_use_pretty_scaling(true);
	} else if(s == "--nomusic") {
		no_music_ = true;
	} else {
		return false;
	}

	return true;
}
}
