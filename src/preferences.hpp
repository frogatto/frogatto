/*
	Copyright (C) 2003-2013 by David White <davewx7@gmail.com>
	
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef PREFERENCES_HPP_INCLUDED
#define PREFERENCES_HPP_INCLUDED

#include <string>

#include "graphics.hpp"
#include "uri.hpp"
#include "variant.hpp"
#if defined(TARGET_OS_HARMATTAN) || defined(TARGET_PANDORA) || defined(TARGET_TEGRA) || defined(TARGET_BLACKBERRY)
#include <EGL/egl.h>
#endif

namespace game_logic {
class formula_callable;
}

#ifdef _WINDOWS
std::string GetAppDataPath();
#endif

namespace preferences {

	const std::string& version();
	const variant& version_decimal();
	int get_unique_user_id();

	bool parse_arg(const char* arg);
	bool no_sound();
	bool no_music();

	void set_preferences_path(const std::string& path);
	void set_preferences_path_from_module(const std::string& name);

	bool setup_preferences_dir();

	const std::string& level_path();
	bool is_level_path_set();
	const char* user_data_path();
	const char* save_file_path();
	const char* auto_save_file_path();
	bool editor_save_to_user_preferences();

	std::string dlc_path();
	void expand_data_paths();
	void set_save_slot(const std::string& fname);
	bool show_debug_hitboxes();
	bool toogle_debug_hitboxes();
	bool show_iphone_controls(); //iphone control hit rects
	bool use_pretty_scaling();
	void set_use_pretty_scaling(bool value);
	bool fullscreen();
	void set_fullscreen(bool value);
	bool no_fullscreen_ever();

	bool resizable();
	bool proportional_resize();
	
	// Reverse A and B buttons for iPhone
	bool reverse_ab();
	void set_reverse_ab(bool value);
	
	// Control scheme to use on iOS or other touch systems
	const std::string& control_scheme();
	void set_control_scheme(const std::string& scheme);
	
	void set_widescreen();
	
	int virtual_screen_width();
	int virtual_screen_height();
	
	int actual_screen_width();
	int actual_screen_height();

	class screen_dimension_override_scope {
		int old_width, old_height, vold_width, vold_height;
	public:
		screen_dimension_override_scope(int width, int height, int vwidth, int vheight);
		~screen_dimension_override_scope();
	};

	//whether we are debugging
	bool debug();
	
	int frame_time_millis();
	bool has_alt_frame_time();

	struct alt_frame_time_scope {
		explicit alt_frame_time_scope(bool value);
		~alt_frame_time_scope();
		bool active() const { return active_; }
	private:
		int old_value_;
		bool active_;
	};

	//whether to show the fps display at the top
	bool show_fps();

	bool use_joystick();

	//load compiled data from data/compiled/
	bool load_compiled();

	void set_load_compiled(bool value);
	
	void set_edit_on_start(bool value);
	bool edit_on_start();

	// Configured language
	const std::string& locale();
	void set_locale(const std::string& value);

#if defined(TARGET_OS_HARMATTAN) || defined(TARGET_PANDORA) || defined(TARGET_TEGRA) || defined(TARGET_BLACKBERRY)
bool use_fbo();
bool use_bequ();
void set_fbo( bool value );
void set_bequ( bool value );
#endif

	//this is the mask which we apply to all x,y values before drawing, to
	//avoid drawing things at "half pixels" when the actual screen dimensions
	//are lower than the virtual screen dimensions.
	extern int xypos_draw_mask;

	bool double_scale();

	//this is a flag set to true iff we are in a mode where we write
	//'compiled' tile output.
	extern bool compiling_tiles;
	
	void set_actual_screen_width(int width);
	void set_actual_screen_height(int height);
	void set_virtual_screen_width(int width);
	void set_virtual_screen_height(int height);
	
	bool allow_autopause();

	bool screen_rotated();
	
	bool force_no_npot_textures();

	bool use_16bpp_textures();

	void set_32bpp_textures_if_kb_memory_at_least(int memory_required );
    
	bool sim_iphone();

	bool no_iphone_controls();

	bool send_stats();

	int force_difficulty();

	bool record_history();
	void set_record_history(bool value);

	bool relay_through_server();
	void set_relay_through_server(bool value);

	variant external_code_editor();

	bool run_failing_unit_tests();

	bool serialize_bad_objects();
	
	bool die_on_assert();

	bool type_safety_checks();

	game_logic::formula_callable* registry();

	void load_preferences();
	void save_preferences();

	uri::uri get_tbs_uri();
	std::string get_username();
	std::string get_password();
	void set_username(const std::string& uname);
	void set_password(const std::string& pword);
	variant get_cookie();
	void set_cookie(const variant &v);

	bool internal_tbs_server();

	class editor_screen_size_scope {
		int width_, height_;
	public:
		editor_screen_size_scope();
		~editor_screen_size_scope();
	};

#if defined(TARGET_OS_HARMATTAN) || defined(TARGET_PANDORA) || defined(TARGET_TEGRA) || defined(TARGET_BLACKBERRY)
	void init_oes( void );
	extern PFNGLBLENDEQUATIONOESPROC           glBlendEquationOES;
	extern PFNGLGENFRAMEBUFFERSOESPROC         glGenFramebuffersOES;
	extern PFNGLBINDFRAMEBUFFEROESPROC         glBindFramebufferOES;
	extern PFNGLFRAMEBUFFERTEXTURE2DOESPROC    glFramebufferTexture2DOES;
	extern PFNGLCHECKFRAMEBUFFERSTATUSOESPROC  glCheckFramebufferStatusOES;
#endif
}

#endif
