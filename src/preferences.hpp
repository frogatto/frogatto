#ifndef PREFERENCES_HPP_INCLUDED
#define PREFERENCES_HPP_INCLUDED

#include <string>

namespace preferences {

	bool parse_arg(const char* arg);
	bool no_sound();
	bool no_music();

	void set_preferences_path(const std::string& path);

	bool setup_preferences_dir();

	const std::string& level_path();
	const char* user_data_path();
	const char* save_file_path();
	const char* auto_save_file_path();
	void expand_data_paths();
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

	//whether we are debugging
	bool debug();

	//whether to show the fps display at the top
	bool show_fps();

	//load compiled data from data/compiled/
	bool load_compiled();

	void set_load_compiled(bool value);

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

	bool send_stats();

	void load_preferences();
	void save_preferences();

	class editor_screen_size_scope {
		int width_, height_;
	public:
		editor_screen_size_scope();
		~editor_screen_size_scope();
	};
}

#endif
