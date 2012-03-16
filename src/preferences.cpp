#include <iostream>
#include <algorithm>
#include <string>
#include "graphics.hpp"

#include <boost/lexical_cast.hpp>

#include "controls.hpp"
#include "filesystem.hpp"
#include "formatter.hpp"
#include "formula_callable.hpp"
#include "game_registry.hpp"
#include "preferences.hpp"
#include "sound.hpp"
#include "wml_node.hpp"
#include <time.h>

#include "wml_parser.hpp"
#include "wml_utils.hpp"
#include "wml_writer.hpp"

#define SAVE_FILENAME					"save.cfg"
#define AUTOSAVE_FILENAME				"autosave.cfg"

#ifdef _WINDOWS
#include <shlobj.h>
#include <shlwapi.h>

class WindowsPrefs
{
public:
	std::string GetPreferencePath()   { if (State* i = Instance()) return i->GetPreferencePath(); }
	std::string GetSaveFilePath()     { if (State* i = Instance()) return i->GetSaveFilePath(); }
	std::string GetAutoSaveFilePath() { if (State* i = Instance()) return i->GetAutoSaveFilePath(); }
	
private:
	struct State 
	{
	private:
		std::string preferences_path;
		std::string save_file_path;
		std::string auto_save_file_path;
	public:
		State::State()
		{
			TCHAR szPath[ MAX_PATH ];
			if( SUCCEEDED( SHGetFolderPath( NULL, CSIDL_APPDATA, NULL, 0, szPath ) ) ) 
			{
				::PathAppend( szPath, TEXT( "\\Frogatto\\" ) );
			}
			this->preferences_path = std::string( szPath );
			this->save_file_path = this->preferences_path + TEXT( SAVE_FILENAME );
			this->auto_save_file_path = this->preferences_path + TEXT( AUTOSAVE_FILENAME );
		}
		std::string GetPreferencePath()
		{
			return this->preferences_path;
		};

		std::string GetSaveFilePath()
		{
			return this->save_file_path;
		}

		std::string GetAutoSaveFilePath()
		{
			return this->auto_save_file_path;
		}
	};

	static State* Instance();
	static void CleanUp();

	static bool MDestroyed;
	static State* MInstance;
};

bool WindowsPrefs::MDestroyed = false;
WindowsPrefs::State* WindowsPrefs::MInstance = 0;

WindowsPrefs::State* WindowsPrefs::Instance()
{
	if( !MDestroyed && !MInstance ) {
		MInstance = new State();
		atexit( &CleanUp );
	}
	return MInstance;
}

void WindowsPrefs::CleanUp()
{
	delete MInstance;
	MInstance = 0;
	MDestroyed = true;
}

WindowsPrefs winPrefs;
#endif // _WINDOWS


namespace preferences {
	const std::string& version() {
		static const std::string Version = "1.2";
		return Version;
	}

	namespace {
		int unique_user_id = 0;

		int screen_editor_mode = 0;

		bool no_sound_ = false;
		bool no_music_ = false;
		bool show_debug_hitboxes_ = false;
		bool show_iphone_controls_ = false;
		bool use_pretty_scaling_ = false;
		bool fullscreen_ = false;
		bool resizable_ = false;
		bool debug_ = true;
		bool reverse_ab_ = false;
		bool show_fps_ = false;
		int frame_time_millis_ = 20;

		std::string level_path_ = "data/level/";

		bool relay_through_server_ = false;
		
		std::string control_scheme_ = "iphone_2d";

		bool record_history_ = false;
		
#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
		
#ifndef PREFERENCES_PATH
#define PREFERENCES_PATH "../Documents/"
#endif
		bool send_stats_ = false;
		
		bool sim_iphone_ = true;

		int virtual_screen_width_ = 960;
		int virtual_screen_height_ = 640;
		
		int actual_screen_width_ = 320;
		int actual_screen_height_ = 480;

		bool screen_rotated_ = true;
		
		bool use_joystick_ = false;
		
		bool load_compiled_ = true;

		bool use_16bpp_textures_ = true;
#elif defined(TARGET_OS_HARMATTAN)

		bool send_stats_ = false;

		bool sim_iphone_ = false;

#ifndef PREFERENCES_PATH
#define PREFERENCES_PATH "~/.frogatto/"
#endif
		int virtual_screen_width_ = 854;
		int virtual_screen_height_ = 480;

		int actual_screen_width_ = 854;
		int actual_screen_height_ = 480;

		bool screen_rotated_ = false;

		bool use_joystick_ = true;

		bool load_compiled_ = true;

		bool use_fbo_ = true;
		bool use_bequ_ = true;

		bool use_16bpp_textures_ = true;
#elif defined(TARGET_BLACKBERRY)

#ifndef PREFERENCES_PATH
#define PREFERENCES_PATH "~/.frogatto/"
#endif
		bool send_stats_ = false;
		bool sim_iphone_ = false;
		bool use_joystick_ = true;
		bool screen_rotated_ = false;
		int virtual_screen_width_ = 1024;
		int virtual_screen_height_ = 600;
		int actual_screen_width_ = 1024;
		int actual_screen_height_ = 600;
		bool load_compiled_ = true;
		bool use_fbo_ = true;
		bool use_bequ_ = true;
		bool use_16bpp_textures_ = false;
#else

#ifndef NO_UPLOAD_STATS
		bool send_stats_ = true;
#else
		bool send_stats_ = false;
#endif
		
		bool sim_iphone_ = false;

#ifndef PREFERENCES_PATH
#define PREFERENCES_PATH "~/.frogatto/"
#endif
		bool screen_rotated_ = false;
		
		bool use_joystick_ = true;

#if defined(TARGET_TEGRA)
		int virtual_screen_width_ = 1024;
		int virtual_screen_height_ = 600;
		
		int actual_screen_width_ = 1024;
		int actual_screen_height_ = 600;
	
		bool load_compiled_ = true;
		bool use_fbo_ = true;
		bool use_bequ_ = true;
#else
		int virtual_screen_width_ = 800;
		int virtual_screen_height_ = 600;
		
		int actual_screen_width_ = 800;
		int actual_screen_height_ = 600;
	
		bool load_compiled_ = false;
#endif

#if defined(TARGET_PANDORA)
        bool use_fbo_ = true;
        bool use_bequ_ = true;
#endif

		bool use_16bpp_textures_ = false;
#endif

		std::string preferences_path_ = PREFERENCES_PATH;
		std::string save_file_path_ = PREFERENCES_PATH SAVE_FILENAME;
		std::string auto_save_file_path_ = PREFERENCES_PATH AUTOSAVE_FILENAME;
		
		bool force_no_npot_textures_ = false;

		bool run_failing_unit_tests_ = false;
	}

	int get_unique_user_id() {
		if(unique_user_id == 0) {
			time_t t1;
			time(&t1);
			int tm = t1;
			unique_user_id = tm^rand();
		}

		return unique_user_id;
	}

	int xypos_draw_mask = actual_screen_width_ < virtual_screen_width_ ? ~1 : ~0;
	bool double_scale() {
		return xypos_draw_mask&1;
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

	bool no_music() {
		return no_music_;
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
#if defined(TARGET_PANDORA)
			str = std::string(getenv("PWD")) + std::string(str.begin()+1, str.end());
#else
			str = std::string(getenv("HOME")) + std::string(str.begin()+1, str.end());
#endif
		}
	}
	}

	void expand_data_paths() {
		expand_path(level_path_);
		expand_path(save_file_path_);
		expand_path(auto_save_file_path_);
		expand_path(preferences_path_);
		std::cerr << "EXPAND DATA PATHS\n";
	}

	void set_save_slot(const std::string& fname) {
		save_file_path_ = std::string(user_data_path()) + "/" + fname;
		std::cerr << "SET SAVE FILE PATH TO " << save_file_path_ << "\n";
	}
	
	bool show_debug_hitboxes() {
		return show_debug_hitboxes_;
	}
	
	bool show_iphone_controls() {
		return show_iphone_controls_;
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

	bool resizable() {
		return resizable_;
	}
	
	bool reverse_ab() {
		return reverse_ab_;
	}
	
	void set_reverse_ab(bool value) {
		reverse_ab_ = value;
	}
	
	const std::string& control_scheme()
	{
		return control_scheme_;
	}
	
	void set_control_scheme(const std::string& scheme)
	{
		control_scheme_ = scheme;
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
	
#if defined(TARGET_OS_HARMATTAN) || defined(TARGET_PANDORA) || defined(TARGET_TEGRA) || defined(TARGET_BLACKBERRY)
	bool use_fbo()
	{
		return use_fbo_;
	}

	bool use_bequ()
	{
		return use_bequ_;
	}

    void set_fbo( bool value )
    {
		use_fbo_ = value;
    }

    void set_bequ( bool value )
    {
		use_bequ_ = value;
    }
#endif

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
	
	int frame_time_millis()
	{
		return frame_time_millis_;
	}

	bool use_joystick()
	{
		return use_joystick_;
	}

	game_logic::formula_callable* registry()
	{
		return &game_registry::instance();
	}

	void load_preferences()
	{
#if defined( _WINDOWS )
		preferences_path_ = winPrefs.GetPreferencePath();
		save_file_path_ = winPrefs.GetSaveFilePath();
		auto_save_file_path_ = winPrefs.GetAutoSaveFilePath();
		std::string path = preferences_path_;
#else
		std::string path = PREFERENCES_PATH;
#endif // defined( _WINDOWS )
		expand_path(path);
		if(!sys::file_exists(path + "preferences.cfg")) {
			return;
		}

		const wml::const_node_ptr node = wml::parse_wml_from_file(path + "preferences.cfg");
		if(node.get() == NULL) {
			return;
		}

		unique_user_id = wml::get_int(node, "user_id", 0);

		use_joystick_ = wml::get_bool(node, "joystick", use_joystick_);

		no_sound_ = wml::get_bool(node, "no_sound", no_sound_);
		no_music_ = wml::get_bool(node, "no_music", no_music_);
		reverse_ab_ = wml::get_bool(node, "reverse_ab", reverse_ab_);

		sound::set_music_volume(wml::get_int(node, "music_volume", 1000)/1000.0);
		sound::set_sound_volume(wml::get_int(node, "sound_volume", 1000)/1000.0);

		const wml::const_node_ptr registry_node = node->get_child("registry");
		if(registry_node) {
			game_registry::instance().set_contents(registry_node);
		}

#if !TARGET_OS_IPHONE && !TARGET_IPHONE_SIMULATOR
		controls::set_sdlkey(controls::CONTROL_UP, static_cast<SDLKey>(wml::get_int(node, "key_up", SDLK_UP)));
		controls::set_sdlkey(controls::CONTROL_DOWN, static_cast<SDLKey>(wml::get_int(node, "key_down", SDLK_DOWN)));
		controls::set_sdlkey(controls::CONTROL_LEFT, static_cast<SDLKey>(wml::get_int(node, "key_left", SDLK_LEFT)));
		controls::set_sdlkey(controls::CONTROL_RIGHT, static_cast<SDLKey>(wml::get_int(node, "key_right", SDLK_RIGHT)));
		controls::set_sdlkey(controls::CONTROL_ATTACK, static_cast<SDLKey>(wml::get_int(node, "key_attack", SDLK_d)));
		controls::set_sdlkey(controls::CONTROL_JUMP, static_cast<SDLKey>(wml::get_int(node, "key_jump", SDLK_a)));
		controls::set_sdlkey(controls::CONTROL_TONGUE, static_cast<SDLKey>(wml::get_int(node, "key_tongue", SDLK_s)));
#endif
	}

	void save_preferences()
	{
		wml::node_ptr node(new wml::node("preferences"));
		node->set_attr("user_id", formatter() << get_unique_user_id());
		node->set_attr("no_sound", no_sound_ ? "true" : "false");
		node->set_attr("no_music", no_music_ ? "true" : "false");
		node->set_attr("reverse_ab", reverse_ab_ ? "true" : "false");
		node->set_attr("joystick", use_joystick_ ? "true" : "false");
		node->set_attr("sound_volume", formatter() << static_cast<int>(sound::get_sound_volume()*1000));
		node->set_attr("music_volume", formatter() << static_cast<int>(sound::get_music_volume()*1000));
		node->set_attr("key_up", formatter() << controls::get_sdlkey(controls::CONTROL_UP));
		node->set_attr("key_down", formatter() << controls::get_sdlkey(controls::CONTROL_DOWN));
		node->set_attr("key_left", formatter() << controls::get_sdlkey(controls::CONTROL_LEFT));
		node->set_attr("key_right", formatter() << controls::get_sdlkey(controls::CONTROL_RIGHT));
		node->set_attr("key_attack", formatter() << controls::get_sdlkey(controls::CONTROL_ATTACK));
		node->set_attr("key_jump", formatter() << controls::get_sdlkey(controls::CONTROL_JUMP));
		node->set_attr("key_tongue", formatter() << controls::get_sdlkey(controls::CONTROL_TONGUE));

		wml::node_ptr registry_node(new wml::node("registry"));
		game_registry::instance().write_contents(registry_node);
		node->add_child(registry_node);
		sys::write_file(preferences_path_ + "preferences.cfg", wml::output(node));
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
		} else if(s == "--show-controls") {
			show_iphone_controls_ = true;
		} else if(s == "--scale") {
			set_use_pretty_scaling(true);
		} else if(s == "--no-sound") {
			no_sound_ = true;
		} else if(s == "--no-music") {
			no_music_ = true;
		} else if(s == "--sound") {
			no_sound_ = false;
		} else if(s == "--music") {
			no_music_ = false;
		} else if(s == "--fullscreen") {
			fullscreen_ = true;
		} else if(s == "--windowed") {
			fullscreen_ = false;
		} else if(s == "--resizable") {
			resizable_ = true;
		} else if(s == "--no-resizable") {
			resizable_ = false;
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
			sim_iphone_ = true;

			virtual_screen_width_ = 960;
			virtual_screen_height_ = 640;
		
			actual_screen_width_ = 480;
			actual_screen_height_ = 320;
			use_16bpp_textures_ = true;

			recalculate_draw_mask();
		} else if(s == "--simipad") {
			sim_iphone_ = true;
			control_scheme_ = "ipad_2d";

			virtual_screen_width_ = 1024;
			virtual_screen_height_ = 768;
		
			actual_screen_width_ = 1024;
			actual_screen_height_ = 768;

			recalculate_draw_mask();
		} else if(s == "--wvga") {
			virtual_screen_width_ = 800;
			virtual_screen_height_ = 480;

			actual_screen_width_ = 800;
			actual_screen_height_ = 480;

			recalculate_draw_mask();
		} else if(s == "--native") {
			virtual_screen_width_ = (actual_screen_width_) * 2;
			virtual_screen_height_ = (actual_screen_height_) * 2;
			recalculate_draw_mask();
		} else if(s == "--fps") {
			show_fps_ = true;
		} else if(s == "--no-fps") {
			show_fps_ = false;
		} else if(arg_name == "--set-fps" && !arg_value.empty()) {
			frame_time_millis_ = 1000/boost::lexical_cast<int, std::string>(arg_value);
			std::cerr << "FPS: " << arg_value << " = " << frame_time_millis_ << "ms/frame\n";
		} else if(arg_name == "--config-path" && !arg_value.empty()) {
			set_preferences_path(arg_value);
		} else if(s == "--send-stats") {
			send_stats_ = true;
		} else if(s == "--no-send-stats") {
			send_stats_ = false;
		} else if(s == "--time-travel") {
			record_history_ = true;
		} else if(s == "--joystick") {
			use_joystick_ = true;
		} else if(s == "--no-joystick") {
			use_joystick_ = false;
		} else if(s == "--relay") {
			relay_through_server_ = true;
		} else if(s == "--failing-tests") {
			run_failing_unit_tests_ = true;
		} else {
			return false;
		}
		
		return true;
	}

	bool use_16bpp_textures() {
		return use_16bpp_textures_;
	}

	bool sim_iphone() {
		return sim_iphone_;
	}

	bool send_stats() {
		return send_stats_;
	}

	bool record_history() {
		return record_history_;
	}

	bool relay_through_server() {
		return relay_through_server_;
	}

	void set_relay_through_server(bool value) {
		relay_through_server_ = value;
	}

	bool run_failing_unit_tests() {
		return run_failing_unit_tests_;
	}

#if defined(TARGET_OS_HARMATTAN) || defined(TARGET_PANDORA) || defined(TARGET_TEGRA) || defined(TARGET_BLACKBERRY)
	PFNGLBLENDEQUATIONOESPROC           glBlendEquationOES;
	PFNGLGENFRAMEBUFFERSOESPROC         glGenFramebuffersOES;
	PFNGLBINDFRAMEBUFFEROESPROC         glBindFramebufferOES;
	PFNGLFRAMEBUFFERTEXTURE2DOESPROC    glFramebufferTexture2DOES;
	PFNGLCHECKFRAMEBUFFERSTATUSOESPROC  glCheckFramebufferStatusOES;

	void init_oes( void )
	{
		int retry;
		const GLubyte* pszGLExtensions = NULL;
		
		glBlendEquationOES          = NULL;
		glGenFramebuffersOES        = NULL;
		glBindFramebufferOES        = NULL;
		glFramebufferTexture2DOES   = NULL;
		glCheckFramebufferStatusOES = NULL;
		
		/* Retrieve GL extension string */
		pszGLExtensions = glGetString(GL_EXTENSIONS);
		
		// Blend subtract Functions
		if(preferences::use_bequ())
		{
			/* GL_OES_framebuffer_object */
			if (strstr((char *)pszGLExtensions, "GL_OES_blend_subtract"))
			{
				for (retry=1; retry<4; retry++)
				{
					glBlendEquationOES = (PFNGLBLENDEQUATIONOESPROC)eglGetProcAddress("glBlendEquationOES");
					if (glBlendEquationOES!=NULL) {
						std::cerr << "glBlendEquationOES was set correctly\n";
						break;
					}
					else
					{
						std::cerr << "glBlendEquationOES was set to NULL by eglGetProcAddress retry #" << retry << "\n";
					}
				}
			}
			else
			{
				glBlendEquationOES = NULL;
			}
		}
		else
		{
			glBlendEquationOES = NULL;
		}
		
		// FBO Functions
		if(preferences::use_fbo())
		{
			/* GL_OES_framebuffer_object */
			if (strstr((char *)pszGLExtensions, "GL_OES_framebuffer_object"))
			{
				for (retry=1; retry<4; retry++)
				{
					glGenFramebuffersOES        = (PFNGLGENFRAMEBUFFERSOESPROC)eglGetProcAddress("glGenFramebuffersOES");
					if (glGenFramebuffersOES!=NULL) {
						std::cerr << "glGenFramebuffersOES was set correctly\n";
						break;
					}
					else
					{
						std::cerr << "glGenFramebuffersOES was set to NULL by eglGetProcAddress retry #" << retry << "\n";
					}
				}
				for (retry=1; retry<4; retry++)
				{
					glBindFramebufferOES        = (PFNGLBINDFRAMEBUFFEROESPROC)eglGetProcAddress("glBindFramebufferOES");
					if (glBindFramebufferOES!=NULL) {
						std::cerr << "glBindFramebufferOES was set correctly\n";
						break;
					}
					else
					{
						std::cerr << "glBindFramebufferOES was set to NULL by eglGetProcAddress retry #" << retry << "\n";
					}
				}
				for (retry=1; retry<4; retry++)
				{
					glFramebufferTexture2DOES   = (PFNGLFRAMEBUFFERTEXTURE2DOESPROC)eglGetProcAddress("glFramebufferTexture2DOES");
					if (glFramebufferTexture2DOES!=NULL) {
						std::cerr << "glFramebufferTexture2DOES was set correctly\n";
						break;
					}
					else
					{
						std::cerr << "glFramebufferTexture2DOES was set to NULL by eglGetProcAddress retry #" << retry << "\n";
					}
				}
				for (retry=1; retry<4; retry++)
				{
					glCheckFramebufferStatusOES = (PFNGLCHECKFRAMEBUFFERSTATUSOESPROC)eglGetProcAddress("glCheckFramebufferStatusOES");
					if (glCheckFramebufferStatusOES!=NULL) {
						std::cerr << "glCheckFramebufferStatusOES was set correctly\n";
						break;
					}
					else
					{
						std::cerr << "glCheckFramebufferStatusOES was set to NULL by eglGetProcAddress retry #" << retry << "\n";
					}
				}
			}
			else
			{
				glGenFramebuffersOES        = NULL;
				glBindFramebufferOES        = NULL;
				glFramebufferTexture2DOES   = NULL;
				glCheckFramebufferStatusOES = NULL;
			}
		}
		else
		{
			glGenFramebuffersOES        = NULL;
			glBindFramebufferOES        = NULL;
			glFramebufferTexture2DOES   = NULL;
			glCheckFramebufferStatusOES = NULL;
		}
	}
#endif
	}
