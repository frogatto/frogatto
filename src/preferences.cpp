#include <iostream>
#include <algorithm>
#include <string>
#include <SDL.h>

#include "preferences.hpp"
#include "filesystem.hpp"

namespace preferences {
	namespace {
		int screen_editor_mode = 0;

		bool no_sound_ = false;
		bool show_debug_hitboxes_ = false;
		bool use_pretty_scaling_ = false;
		bool fullscreen_ = false;
		bool debug_ = true;

		std::string level_path_ = "data/level/";

		bool send_stats_ = true;
		
#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
		
#ifndef PREFERENCES_PATH
#define PREFERENCES_PATH "../Documents/"
#endif

		int virtual_screen_width_ = 960;
		int virtual_screen_height_ = 640;
		
		int actual_screen_width_ = 320;
		int actual_screen_height_ = 480;

		bool screen_rotated_ = true;

		bool show_fps_ = false;
		
		bool load_compiled_ = true;

		bool use_16bpp_textures_ = true;
#else

#ifndef PREFERENCES_PATH
#define PREFERENCES_PATH "~/.frogatto/"
#endif
		int virtual_screen_width_ = 800;
		int virtual_screen_height_ = 600;
		
		int actual_screen_width_ = 800;
		int actual_screen_height_ = 600;
		
		bool screen_rotated_ = false;

		bool show_fps_ = false;

		bool load_compiled_ = false;

		bool use_16bpp_textures_ = false;
#endif

#define SAVE_FILENAME					"save.cfg"
#define AUTOSAVE_FILENAME				"autosave.cfg"

		std::string preferences_path_ = PREFERENCES_PATH;
		std::string save_file_path_ = PREFERENCES_PATH SAVE_FILENAME;
		std::string auto_save_file_path_ = PREFERENCES_PATH AUTOSAVE_FILENAME;
		
		bool force_no_npot_textures_ = false;
	}

	int xypos_draw_mask = actual_screen_width_ < virtual_screen_width_ ? ~1 : ~0;
	bool double_scale() {
		return xypos_draw_mask&1 != 0;
	}
	bool compiling_tiles = false;

	namespace {
	void recalculate_draw_mask() {
		xypos_draw_mask = actual_screen_width_ < virtual_screen_width_ ? ~1 : ~0;
	}
	}
	
	bool no_sound() {
		return no_sound_;
	}

	bool setup_preferences_dir()
	{
		return !sys::get_dir(user_data_path()).empty();
	}

	void set_preferences_path(const std::string& path)
	{
		preferences_path_ = path;
		if(preferences_path_[preferences_path_.length()-1] != '/') {
			preferences_path_ += '/';
		}

		save_file_path_ = preferences_path_ + SAVE_FILENAME;
		auto_save_file_path_ = preferences_path_ + AUTOSAVE_FILENAME;	
	}

	const std::string& level_path() {
		return level_path_;
	}
	
	const char *save_file_path() {
		return save_file_path_.c_str();
	}

	const char *auto_save_file_path() {
		return auto_save_file_path_.c_str();
	}

	const char *user_data_path() {
		return preferences_path_.c_str();
	}

	namespace {
	void expand_path(std::string& str) {
		if(!str.empty() && str[0] == '~') {
			str = std::string(getenv("HOME")) + std::string(str.begin()+1, str.end());
		}
	}
	}

	void expand_data_paths() {
		expand_path(level_path_);
		expand_path(save_file_path_);
		expand_path(auto_save_file_path_);
		expand_path(preferences_path_);
		std::cerr << "PATH: " << save_file_path_ << "\n";
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
		recalculate_draw_mask();
	}
	
	int virtual_screen_width()
	{
		return virtual_screen_width_;
	}
	
	int virtual_screen_height()
	{
		return virtual_screen_height_;
	}
	
	void set_virtual_screen_width (int width)
	{
		virtual_screen_width_ = width;
		recalculate_draw_mask();
	}
	
	void set_virtual_screen_height (int height)
	{
		virtual_screen_height_ = height;
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
		if(screen_editor_mode) {
			virtual_screen_width_ = actual_screen_width_;
		}
		recalculate_draw_mask();
	}
	
	void set_actual_screen_height(int height)
	{
		actual_screen_height_ = height;
		if(screen_editor_mode) {
			virtual_screen_height_ = actual_screen_height_;
		}
	}

	bool load_compiled()
	{
		return load_compiled_;
	}

	void set_load_compiled(bool value)
	{
		load_compiled_ = value;
	}
	
	bool force_no_npot_textures()
	{
		return force_no_npot_textures_;
	}
	
	bool screen_rotated()
	{
		return screen_rotated_;
	}

	bool debug()
	{
		return debug_;
	}

	bool show_fps()
	{
		return show_fps_;
	}

	editor_screen_size_scope::editor_screen_size_scope() : width_(virtual_screen_width_), height_(virtual_screen_height_) {
		++screen_editor_mode;
		virtual_screen_width_ = actual_screen_width_;
		virtual_screen_height_ = actual_screen_height_;
	}

	editor_screen_size_scope::~editor_screen_size_scope() {
		virtual_screen_width_ = width_;
		virtual_screen_height_ = height_;
		--screen_editor_mode;
	}
	
	bool parse_arg(const char* arg) {
		const std::string s(arg);

		std::string arg_name, arg_value;
		std::string::const_iterator equal = std::find(s.begin(), s.end(), '=');
		if(equal != s.end()) {
			arg_name = std::string(s.begin(), equal);
			arg_value = std::string(equal+1, s.end());
		}

		if(arg_name == "--level-path") {
			level_path_ = arg_value + "/";
		} else if(s == "--show-hitboxes") {
			show_debug_hitboxes_ = true;
		} else if(s == "--scale") {
			set_use_pretty_scaling(true);
		} else if(s == "--no-sound") {
			no_sound_ = true;
		} else if(s == "--fullscreen") {
			fullscreen_ = true;
		} else if(s == "--widescreen") {
			set_widescreen();
		} else if(s == "--bigscreen") {
			virtual_screen_width_ = actual_screen_width_;
			virtual_screen_height_ = actual_screen_height_;
		} else if(s == "--potonly") {
			force_no_npot_textures_ = true;
		} else if(s == "--textures16") {
			use_16bpp_textures_ = true;
		} else if(s == "--textures32") {
			use_16bpp_textures_ = false;
		} else if(s == "--debug") {
			debug_ = true;
		} else if(s == "--no-debug") {
			debug_ = false;
		} else if(s == "--simiphone") {
			virtual_screen_width_ = 960;
			virtual_screen_height_ = 640;
		
			actual_screen_width_ = 480;
			actual_screen_height_ = 320;
			use_16bpp_textures_ = true;

			recalculate_draw_mask();
		} else if(s == "--simipad") {
			virtual_screen_width_ = 1024;
			virtual_screen_height_ = 768;
		
			actual_screen_width_ = 1024;
			actual_screen_height_ = 768;

			recalculate_draw_mask();
		} else if(s == "--fps") {
			show_fps_ = true;
		} else if(s == "--no-fps") {
			show_fps_ = false;
		} else if(arg_name == "--config-path" && !arg_value.empty()) {
			set_preferences_path(arg_value);
		} else if(s == "--no-send-stats") {
			send_stats_ = false;
		} else {
			return false;
		}
		
		return true;
	}

	bool use_16bpp_textures() {
		return use_16bpp_textures_;
	}

	bool send_stats() {
		return send_stats_;
	}
}
