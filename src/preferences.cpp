#include <string>

#include "preferences.hpp"

namespace preferences {
namespace {
bool show_debug_hitboxes_ = false;
}

bool show_debug_hitboxes() {
	return show_debug_hitboxes_;
}

bool parse_arg(const char* arg) {
	std::string s(arg);
	if(s == "--show_hitboxes") {
		show_debug_hitboxes_ = true;
	} else {
		return false;
	}

	return true;
}
}
