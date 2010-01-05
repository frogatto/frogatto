#ifndef PREFERENCES_HPP_INCLUDED
#define PREFERENCES_HPP_INCLUDED

namespace preferences {
	
	bool parse_arg(const char* arg);
	bool no_sound();
	const char* save_file_path();
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

	//load compiled data from data/compiled/
	bool load_compiled();

	void set_load_compiled(bool value);

	//this is the mask which we apply to all x,y values before drawing, to
	//avoid drawing things at "half pixels" when the actual screen dimensions
	//are lower than the virtual screen dimensions.
	extern int xypos_draw_mask;

	//this is a flag set to true iff we are in a mode where we write
	//'compiled' tile output.
	extern bool compiling_tiles;
	
	void set_actual_screen_width(int width);
	void set_actual_screen_height(int height);
	
	bool screen_rotated();
	
	bool force_no_npot_textures();
}

#endif
