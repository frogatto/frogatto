#include <string>

#include "preferences.hpp"

namespace preferences {
namespace {
bool no_sound_ = false;
bool show_debug_hitboxes_ = false;
bool use_pretty_scaling_ = false;
bool fullscreen_ = false;

#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
int virtual_screen_width_ = 960;
int virtual_screen_height_ = 600;

int actual_screen_width_ = 960;
int actual_screen_height_ = 600;
#else
int virtual_screen_width_ = 800;
int virtual_screen_height_ = 600;

int actual_screen_width_ = 800;
int actual_screen_height_ = 600;
#endif

bool force_no_npot_textures_ = false;
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

void set_widescreen()
{
	virtual_screen_width_ = (virtual_screen_height_*16)/9;
	actual_screen_width_ = (actual_screen_height_*16)/9;
}

int virtual_screen_width()
{
	return virtual_screen_width_;
}

int virtual_screen_height()
{
	return virtual_screen_height_;
}

int actual_screen_width()
{
	return actual_screen_width_;
}

int actual_screen_height()
{
	return actual_screen_height_;
}

void set_actual_screen_width(int width)
{
	actual_screen_width_ = width;
}

void set_actual_screen_height(int height)
{
	actual_screen_height_ = height;
}

bool force_no_npot_textures()
{
	return force_no_npot_textures_;
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
	} else if(s == "--widescreen") {
		set_widescreen();
	} else if(s == "--potonly") {
		force_no_npot_textures_ = true;
	} else {
		return false;
	}

	return true;
}
}
