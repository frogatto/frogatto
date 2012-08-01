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
	void set_preferences_path_from_module( const std::string& name);

	bool setup_preferences_dir();

	const std::string& level_path();
	bool is_level_path_set();
	const char* user_data_path();
	const char* save_file_path();
	const char* auto_save_file_path();
	void expand_data_paths();
	void set_save_slot(const std::string& fname);
	bool show_debug_hitboxes();
	bool toogle_debug_hitboxes();
	bool show_iphone_controls(); //iphone control hit rects
	bool use_pretty_scaling();
	void set_use_pretty_scaling(bool value);
	bool fullscreen();
	void set_fullscreen(bool value);

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

	//whether we are debugging
	bool debug();
	
	int frame_time_millis();

	//whether to show the fps display at the top
	bool show_fps();

	bool use_joystick();

	//load compiled data from data/compiled/
	bool load_compiled();

	void set_load_compiled(bool value);
	
	void set_edit_on_start(bool value);
	bool edit_on_start();

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
	
	bool screen_rotated();
	
	bool force_no_npot_textures();

	bool use_16bpp_textures();

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

	game_logic::formula_callable* registry();

	void load_preferences();
	void save_preferences();

	uri::uri get_tbs_uri();
	std::string get_username();
	std::string get_password();

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
