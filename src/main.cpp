#include "graphics.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <cstdio>

#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/scoped_ptr.hpp>

#ifdef TARGET_OS_HARMATTAN
#include <glib-object.h>
#endif

#include "asserts.hpp"
#include "background_task_pool.hpp"
#include "checksum.hpp"
#include "controls.hpp"
#include "custom_object.hpp"
#include "custom_object_functions.hpp"
#include "custom_object_type.hpp"
#include "draw_scene.hpp"
#ifndef NO_EDITOR
#include "editor.hpp"
#endif
#include "difficulty.hpp"
#include "external_text_editor.hpp"
#include "filesystem.hpp"
#include "font.hpp"
#include "foreach.hpp"
#include "formula_profiler.hpp"
#include "framed_gui_element.hpp"
#include "graphical_font.hpp"
#include "gui_section.hpp"
#include "i18n.hpp"
#include "iphone_device_info.h"
#include "of_bridge.h"
#include "joystick.hpp"
#include "json_parser.hpp"
#include "key.hpp"
#include "level.hpp"
#include "level_object.hpp"
#include "level_runner.hpp"
#include "load_level.hpp"
#include "loading_screen.hpp"
#include "message_dialog.hpp"
#include "module.hpp"
#include "multiplayer.hpp"
#include "player_info.hpp"
#include "preferences.hpp"
#include "preprocessor.hpp"
#include "raster.hpp"
#include "sound.hpp"
#include "stats.hpp"
#include "string_utils.hpp"
#include "surface_cache.hpp"
#include "texture.hpp"
#include "texture_frame_buffer.hpp"
#include "tile_map.hpp"
#include "unit_test.hpp"
#include "variant_utils.hpp"
#include "wm.hpp"

#if defined(__ANDROID__)
#include "prof.h"
#endif

#if defined(TARGET_PANDORA) || defined(TARGET_TEGRA)
#include "eglport.h"
#elif defined(TARGET_BLACKBERRY)
#include <EGL/egl.h>
#endif

#define DEFAULT_MODULE	"frogatto"

#if defined(USE_GLES2)
window_manager wm;
#endif

namespace {

bool show_title_screen(std::string& level_cfg)
{
	//currently the titlescreen is disabled.
	return false;
}

void print_help(const std::string& argv0)
{
	std::cout << "Usage: " << argv0 << " [OPTIONS]\n" <<
"\n" <<
"User options:\n" <<
//"      --bigscreen              FIXME\n" <<
"      --config-path=PATH       sets the path to the user config dir\n" <<
"      --fullscreen             starts in fullscreen mode\n" <<
"      --height NUM             sets the game window height to which contents\n" <<
"                                 are scaled\n" <<
"      --host                   set the game server host address\n" <<
"      --[no-]joystick          enables/disables joystick support\n" <<
"      --level LEVEL_FILE       starts the game using the specified level file,\n" <<
"                                 relative to the level path\n" <<
"      --level-path=PATH        sets the path to the game level files\n" <<
"      --[no-]music             enables/disables game music\n" <<
"      --native                 one pixel in-game equals one pixel on monitor\n" <<
"      --relay                  use the server as a relay in multiplayer rather\n" <<
"                                 than trying to initiate direct connections\n" <<
"      --[no-]resizable         allows/disallows to resize the game window\n" <<
"      --scale                  enables an experimental pixel art interpolation\n" <<
"                                 algorithm for scaling the game graphics (some\n" <<
"                                 issues with this still have to be solved)\n" <<
"      --[no-]send-stats        enables/disables sending game statistics over\n"
"                                 the network\n" <<
"      --server=URL             sets the server to use for the TBS client based\n"
"                                 on the given url\n" <<
"      --user=USERNAME          sets the username to use as part of the TBS\n"
"                                 server and module system\n" <<
"      --pass=PASSWORD          sets the password to use as part of the TBS\n"
"                                 server and module system\n" <<
"      --[no-]sound             enables/disables sound and music support\n" <<
"      --widescreen             sets widescreen mode, increasing the game view\n" <<
"                                 area for wide screen displays\n" <<
"      --width NUM              sets the game window width to which contents are\n" <<
"                                 scaled\n" <<
"      --windowed               starts in windowed mode\n" <<
"      --wvga                   sets the display size to 800x480\n" <<
"\n" <<
"Diagnostic options:\n" <<
"      --[no-]debug             enables/disables debug mode\n" <<
"      --[no-]fps               enables/disables framerate display\n" <<
"      --set-fps=FPS            sets the framerate to FPS\n" <<
"      --potonly                use power of two-sized textures only\n" <<
"      --textures16             use 16 bpp textures only (default on iPhone)\n" <<
"      --textures32             use 32 bpp textures (default on PC/Mac)\n" <<
"\n" <<
"Developer options:\n" <<
"      --benchmarks             runs all the engine's benchmarks (intended to\n" <<
"                                 measure the speed of certain low-level\n" <<
"                                 functions), only useful if you're actually\n" <<
"                                 hacking on the engine to optimize the speed\n" <<
"                                 of these\n" <<
"      --benchmarks=NAME        runs a single named benchmark code\n" <<
"      --[no-]compiled          enable or disable precompiled game data\n" <<
"      --edit                   starts the game in edit mode.\n" <<
//"      --profile                FIXME\n" <<
//"      --profile=FILE           FIXME\n" <<
"      --show-hitboxes          turns on the display of object hitboxes\n" <<
"      --show-controls          turns on the display of iPhone control hitboxes\n" <<
"      --simipad                changes various options to emulate an iPad\n" <<
"                                 environment\n" <<
"      --simiphone              changes various options to emulate an iPhone\n" <<
"                                 environment\n" <<
"      --no-autopause           Stops the game from pausing automatically\n" <<
"                                 when it loses focus\n" <<
"      --tests                  runs the game's unit tests and exits\n" <<
"      --no-tests               skips the execution of unit tests on startup\n"
"      --utility=NAME           runs the specified UTILITY( NAME ) code block,\n" <<
"                                 such as compile_levels or compile_objects,\n" <<
"                                 with the specified arguments\n"
	;
}

}

#if defined(__ANDROID__)
#include <jni.h>
#include <android/asset_manager_jni.h>
AAssetManager* static_assetManager = 0;
extern "C" void app_set_asset_manager(AAssetManager* assetMan)
{
	static_assetManager = assetMan;	
}
namespace sys {
AAssetManager* GetJavaAssetManager()
{
	return static_assetManager;
}
}
#endif

#if defined(__native_client__)
void register_file_and_data(const char* filename, const char* mode, char* buffer, int size)
{
}
#endif

int load_module(const std::string& mod, std::vector<std::string>* argv)
{
	variant mod_info = module::get(mod);
	if(mod_info.is_null()) {
		return -1;
	}
	module::reload(mod);
	if(mod_info["arguments"].is_list()) {
		const std::vector<std::string>& arguments = mod_info["arguments"].as_list_string();
		argv->insert(argv->end(), arguments.begin(), arguments.end());
	}
	preferences::set_preferences_path_from_module(module::get_module_name());
	return 0;
}

#if defined(__native_client__)
extern "C" int game_main(int argcount, char* argvec[])
#else
extern "C" int main(int argcount, char** argvec)
#endif
{
#if defined(__native_client__)
	std::cerr << "Running game_main" << std::endl;

	chdir("/frogatto");
	{
		char buf[256];
		const char* const res = getcwd(buf,sizeof(buf));
		std::cerr << "Current working directory: " << res << std::endl;
	}
#endif 

	#ifdef NO_STDERR
	std::freopen("/dev/null", "w", stderr);
	std::cerr.sync_with_stdio(true);
	#endif

	std::cerr << "Frogatto engine version " << preferences::version() << "\n";
	LOG( "After print engine version" );

	#if defined(TARGET_BLACKBERRY)
		chdir("app/native");
		std::cout<< "Changed working directory to: " << getcwd(0, 0) << std::endl;
	#endif

	std::string level_cfg = "titlescreen.cfg";
	bool unit_tests_only = false, skip_tests = false;
	bool run_benchmarks = false;
	std::vector<std::string> benchmarks_list;
	std::string utility_program;
	std::vector<std::string> util_args;
	std::string server = "wesnoth.org";

	const char* profile_output = NULL;
	std::string profile_output_buf;

#if defined(__ANDROID__)
	monstartup("libapplication.so");
#endif

	std::string orig_level_cfg = level_cfg;
	std::string override_level_cfg = "";

	int modules_loaded = 0;

	std::vector<std::string> argv;
	for(int n = 1; n < argcount; ++n) {
		argv.push_back(argvec[n]);
        
        if(argv.size() >= 2 && argv[argv.size()-2] == "-NSDocumentRevisionsDebugMode" && argv.back() == "YES") {
            //XCode passes these arguments by default when debugging -- make sure they are ignored.
            argv.resize(argv.size()-2);
        }
	}

	if(sys::file_exists("./master-config.cfg")) {
		std::cerr << "LOADING CONFIGURATION FROM master-config.cfg" << std::endl;
		variant cfg = json::parse_from_file("./master-config.cfg");
		if(cfg.is_map()) {
			if( cfg["id"].is_null() == false) {
				std::cerr << "SETTING MODULE PATH FROM master-config.cfg: " << cfg["id"].as_string() << std::endl;
				preferences::set_preferences_path_from_module(cfg["id"].as_string());
				//XXX module::set_module_name(cfg["id"].as_string(), cfg["id"].as_string());
			}
			if(cfg["arguments"].is_null() == false) {
				std::vector<std::string> additional_args = cfg["arguments"].as_list_string();
				argv.insert(argv.begin(), additional_args.begin(), additional_args.end());
				std::cerr << "ADDING ARGUMENTS FROM master-config.cfg:";
				for(size_t n = 0; n < cfg["arguments"].num_elements(); ++n) {
					std::cerr << " " << cfg["arguments"][n].as_string();
				}
				std::cerr << std::endl;
			}
		}
	}

	stats::record_program_args(argv);

	for(size_t n = 0; n < argv.size(); ++n) {
		const int argc = argv.size();
		const std::string arg(argv[n]);
		std::string arg_name, arg_value;
		std::string::const_iterator equal = std::find(arg.begin(), arg.end(), '=');
		if(equal != arg.end()) {
			arg_name = std::string(arg.begin(), equal);
			arg_value = std::string(equal+1, arg.end());
		}
		if(arg_name == "--module") {
			if(load_module(arg_value, &argv) != 0) {
				std::cerr << "FAILED TO LOAD MODULE: " << arg_value << "\n";
				return -1;
			}
			++modules_loaded;
		}
	}

	if(modules_loaded == 0) {
		if(load_module(DEFAULT_MODULE, &argv) != 0) {
			std::cerr << "FAILED TO LOAD MODULE: " << DEFAULT_MODULE << "\n";
			return -1;
		}
	}

	preferences::load_preferences();
	LOG( "After load_preferences()" );

	// load difficulty settings after module, before rest of args.
	difficulty::manager();

	for(size_t n = 0; n < argv.size(); ++n) {
		const size_t argc = argv.size();
		const std::string arg(argv[n]);
		std::string arg_name, arg_value;
		std::string::const_iterator equal = std::find(arg.begin(), arg.end(), '=');
		if(equal != arg.end()) {
			arg_name = std::string(arg.begin(), equal);
			arg_value = std::string(equal+1, arg.end());
		}
		
		if(arg_name == "--module") {
			// ignore already processed.
		} else if(arg_name == "--profile" || arg == "--profile") {
			profile_output_buf = arg_value;
			profile_output = profile_output_buf.c_str();
		} else if(arg_name == "--utility") {
			utility_program = arg_value;
			for(++n; n < argc; ++n) {
				const std::string arg(argv[n]);
				util_args.push_back(arg);
			}

			break;
		} else if(arg == "--benchmarks") {
			run_benchmarks = true;
		} else if(arg_name == "--benchmarks") {
			run_benchmarks = true;
			benchmarks_list = util::split(arg_value);
		} else if(arg == "--tests") {
			unit_tests_only = true;
		} else if(arg == "--no-tests") {
			skip_tests = true;
		} else if(arg == "--width" && n+1 < argc) {
			std::string w(argv[++n]);
			preferences::set_actual_screen_width(boost::lexical_cast<int>(w));
		} else if(arg == "--height" && n+1 < argc) {
			std::string h(argv[++n]);
			preferences::set_actual_screen_height(boost::lexical_cast<int>(h));
		} else if(arg == "--level" && n+1 < argc) {
			override_level_cfg = argv[++n];
		} else if(arg == "--host" && n+1 < argc) {
			server = argv[++n];
		} else if(arg == "--compiled") {
			preferences::set_load_compiled(true);
#ifndef NO_EDITOR
		} else if(arg == "--edit") {
			preferences::set_edit_on_start(true);
#endif
		} else if(arg == "--no-compiled") {
			preferences::set_load_compiled(false);
#if defined(TARGET_PANDORA)
		} else if(arg == "--no-fbo") {
			preferences::set_fbo(false);
		} else if(arg == "--no-bequ") {
			preferences::set_bequ(false);
#endif
		} else if(arg == "--help" || arg == "-h") {
			print_help(std::string(argvec[0]));
			return 0;
		} else {
			const bool res = preferences::parse_arg(argv[n].c_str());
			if(!res) {
				std::cerr << "unrecognized arg: '" << arg << "'\n";
				return -1;
			}
		}
	}

	checksum::manager checksum_manager;
#ifndef NO_EDITOR
	sys::filesystem_manager fs_manager;
#endif // NO_EDITOR

	preferences::expand_data_paths();

	background_task_pool::manager bg_task_pool_manager;
	LOG( "After expand_data_paths()" );

	std::cerr << "Preferences dir: " << preferences::user_data_path() << '\n';

	//make sure that the user data path exists.
	if(!preferences::setup_preferences_dir()) {
		std::cerr << "cannot create preferences dir!\n";
	}

	std::cerr << "\n";

	if(utility_program.empty() == false && test::utility_needs_video(utility_program) == false) {
		test::run_utility(utility_program, util_args);
		return 0;
	}

#if defined(TARGET_PANDORA)
    EGL_Open();
#endif

#if defined(__ANDROID__)
	std::freopen("stdout.txt","w",stdout);
	std::freopen("stderr.txt","w",stderr);
	std::cerr.sync_with_stdio(true);
#endif

	LOG( "Start of main" );
	
#if !defined(__native_client__)
	Uint32 sdl_init_flags = SDL_INIT_VIDEO | SDL_INIT_JOYSTICK;
#if defined(_WINDOWS) || defined(TARGET_OS_IPHONE)
	sdl_init_flags |= SDL_INIT_TIMER;
#endif
	if(SDL_Init(sdl_init_flags) < 0) {
		std::cerr << "could not init SDL\n";
		return -1;
	}
	LOG( "After SDL_Init" );
#endif

#ifdef TARGET_OS_HARMATTAN
	g_type_init();
#endif
	i18n::init ();
	LOG( "After i18n::init()" );

	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

#if defined(TARGET_OS_IPHONE) || defined(TARGET_BLACKBERRY) || defined(__ANDROID__)
	//on the iPhone and PlayBook, try to restore the auto-save if it exists
	if(sys::file_exists(preferences::auto_save_file_path()) && sys::read_file(std::string(preferences::auto_save_file_path()) + ".stat") == "1") {
		level_cfg = "autosave.cfg";
		sys::write_file(std::string(preferences::auto_save_file_path()) + ".stat", "0");

	}
#endif

	if(override_level_cfg.empty() != true) {
		level_cfg = override_level_cfg;
		orig_level_cfg = level_cfg;
	}

#if defined(USE_GLES2)
	wm.create_window(preferences::actual_screen_width(),
		preferences::actual_screen_height(),
		0,
		(preferences::resizable() ? SDL_RESIZABLE : 0) | (preferences::fullscreen() ? SDL_FULLSCREEN : 0));
#else

#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
	int width, height;
	iphone_screen_res(&width, &height);
	preferences::set_actual_screen_width(width);
	preferences::set_actual_screen_height(height);
	int multiplier = 2;
	if (width > 320)
	{
		//preferences::set_use_pretty_scaling(true);
		multiplier = 1;
	}
	preferences::set_virtual_screen_width(height*multiplier);
	preferences::set_virtual_screen_height(width*multiplier);
	preferences::set_control_scheme(height % 1024 ? "iphone_2d" : "ipad_2d");
	
	SDL_WindowID windowID = SDL_CreateWindow (NULL, 0, 0, preferences::actual_screen_width(), preferences::actual_screen_height(),
		SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN |
		SDL_WINDOW_BORDERLESS);
	if (windowID == 0) { 
		std::cerr << "Could not create window: " << SDL_GetError() << "\n"; 
		return -1;
	}
	
	//	if (SDL_GL_CreateContext(windowID) == 0) {
	//		std::cerr << "Could not create GL context: " << SDL_GetError() << "\n";
	//		return -1;
	//	}
	if (SDL_CreateRenderer(windowID, -1, 0) != 0) {
		std::cerr << "Could not create renderer\n";
		return -1;
	}
	
#else
#ifdef TARGET_OS_HARMATTAN
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1);
	if (SDL_SetVideoMode(preferences::actual_screen_width(),preferences::actual_screen_height(),0,SDL_OPENGLES | SDL_FULLSCREEN) == NULL) {
		std::cerr << "could not set video mode\n";
		return -1;
	}

	preferences::init_oes();
	SDL_ShowCursor(0);
#else
#ifndef __APPLE__
	graphics::surface wm_icon = graphics::surface_cache::get("window-icon.png");
	if(!wm_icon.null()) {
		SDL_WM_SetIcon(wm_icon, NULL);
	}
#endif

#if defined(TARGET_PANDORA)
	if (SDL_SetVideoMode(preferences::actual_screen_width(),preferences::actual_screen_height(),16,SDL_FULLSCREEN) == NULL) {
		std::cerr << "could not set video mode\n";
		return -1;
	}
    EGL_Init();
    preferences::init_oes();
#elif defined(TARGET_TEGRA)
	//if (SDL_SetVideoMode(preferences::actual_screen_width(),preferences::actual_screen_height(),0,preferences::resizable() ? SDL_RESIZABLE : 0|preferences::fullscreen() ? SDL_FULLSCREEN : 0) == NULL) {
	if (SDL_SetVideoMode(preferences::actual_screen_width(),preferences::actual_screen_height(),0,SDL_FULLSCREEN) == NULL) {
		std::cerr << "could not set video mode\n";
		return -1;
	}
    EGL_Init();
    preferences::init_oes();
#elif defined(TARGET_BLACKBERRY)
	if (SDL_SetVideoMode(preferences::actual_screen_width(),preferences::actual_screen_height(),0,SDL_OPENGL|SDL_FULLSCREEN) == NULL) {
		std::cerr << "could not set video mode\n";
		return -1;
	}
	preferences::init_oes();
#elif defined(__ANDROID__)
    SDL_Rect** r = SDL_ListModes(NULL, SDL_FULLSCREEN|SDL_OPENGL);
    if( r != (SDL_Rect**)0 && r != (SDL_Rect**)-1 ) {
        preferences::set_actual_screen_width(r[0]->w);
        preferences::set_actual_screen_height(r[0]->h);
		if(r[0]->w < 640) {
        	preferences::set_virtual_screen_width(r[0]->w*2);
        	preferences::set_virtual_screen_height(r[0]->h*2);
		} else {
			preferences::set_virtual_screen_width(r[0]->w);
			preferences::set_virtual_screen_height(r[0]->h);
		}
		preferences::set_control_scheme(r[0]->h >= 1024 ? "ipad_2d" : "android_med");
    }

    if (SDL_SetVideoMode(preferences::actual_screen_width(),preferences::actual_screen_height(),0,SDL_FULLSCREEN|SDL_OPENGL) == NULL) {
		std::cerr << "could not set video mode\n";
		return -1;
    }
#elif defined(__native_client__)
    SDL_Rect** r = SDL_ListModes(NULL, SDL_OPENGL);
	std::cerr << "Video modes";
	if(r == (SDL_Rect**)0) {
		std::cerr << "No modes available";
		return -1;
	}
	if(r == (SDL_Rect**)-1) {
		std::cerr << "All modes available";
	} else {
		for(int i = 0; r[i]; ++i) {
			std::cerr << r[i]->w << r[i]->h << std::endl;
		}
	}

    if (SDL_SetVideoMode(preferences::actual_screen_width(),preferences::actual_screen_height(),0,0) == NULL) {
		std::cerr << "could not set video mode\n";
		return -1;
    }
#else
	if (SDL_SetVideoMode(preferences::actual_screen_width(),preferences::actual_screen_height(),0,SDL_OPENGL|(preferences::resizable() ? SDL_RESIZABLE : 0)|(preferences::fullscreen() ? SDL_FULLSCREEN : 0)) == NULL) {
		std::cerr << "could not set video mode\n";
		return -1;
	}
#endif
#endif

#endif

#endif // USE_GLES2

#if defined(__GLEW_H__)
	GLenum glew_status = glewInit();
	ASSERT_EQ(glew_status, GLEW_OK);
#endif

#if defined(USE_GLES2)
	if(glCreateShader == NULL) {
		const GLubyte* glstrings;
		if(glGetString && (glstrings = glGetString(GL_VERSION)) != NULL) {
			std::cerr << "OpenGL version: " << reinterpret_cast<const char *>(glstrings) << std::endl;
		}
		std::cerr << "glCreateShader is NULL. Check that your current video card drivers support"
			<< "an OpenGL version >= 2. Exiting." << std::endl;
		return 0;
	}
	// Has to happen after the call to glewInit().
	gles2::init_default_shader();
#endif

//	srand(time(NULL));

	const stats::manager stats_manager;
#ifndef NO_EDITOR
	const external_text_editor::manager editor_manager;
#endif // NO_EDITOR

	std::cerr << std::endl;
	const GLubyte* glstrings;
	if((glstrings = glGetString(GL_VENDOR)) != NULL) {
		std::cerr << "OpenGL vendor: " << reinterpret_cast<const char *>(glstrings) << std::endl;
	} else {
		GLenum err = glGetError();
		std::cerr << "Error in vendor string: " << std::hex << err << std::endl;
	}
	if((glstrings = glGetString(GL_VERSION)) != NULL) {
		std::cerr << "OpenGL version: " << reinterpret_cast<const char *>(glstrings) << std::endl;
	} else {
		GLenum err = glGetError();
		std::cerr << "Error in version string: " << std::hex << err << std::endl;
	}
	if((glstrings = glGetString(GL_EXTENSIONS)) != NULL) {
		std::cerr << "OpenGL extensions: " << reinterpret_cast<const char *>(glstrings) << std::endl;
	} else {
		GLenum err = glGetError();
		std::cerr << "Error in extensions string: " << std::hex << err << std::endl;
	}
#ifdef GL_SHADING_LANGUAGE_VERSION
	if((glstrings = glGetString(GL_SHADING_LANGUAGE_VERSION)) != NULL) {
		std::cerr << "GLSL Version: " << reinterpret_cast<const char *>(glstrings) << std::endl;
	} else {
		GLenum err = glGetError();
		std::cerr << "Error in GLSL string: " << std::hex << err << std::endl;
	}
#endif
	std::cerr << std::endl;

	GLint stencil_bits = 0;
	glGetIntegerv(GL_STENCIL_BITS, &stencil_bits);
	std::cerr << "Stencil bits: " << stencil_bits << std::endl;

#if defined(USE_GLES2)
	GLfloat min_pt_sz;
	glGetFloatv(GL_POINT_SIZE_MIN, &min_pt_sz);
	GLfloat max_pt_sz;
	glGetFloatv(GL_POINT_SIZE_MAX, &max_pt_sz);
	std::cerr << "Point size range: " << min_pt_sz << " < size < " << max_pt_sz << std::endl;
#if !defined(GL_ES_VERSION_2_0)
	glEnable(GL_POINT_SPRITE);
	glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
#endif
#endif

	glShadeModel(GL_SMOOTH);
	glEnable(GL_BLEND);
#if !defined(USE_GLES2)
	glEnable(GL_TEXTURE_2D);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
#endif

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	SDL_WM_SetCaption(module::get_module_pretty_name().c_str(), module::get_module_pretty_name().c_str());

	std::cerr << "JOYSTICKS: " << SDL_NumJoysticks() << "\n";

	const load_level_manager load_manager;

	{ //manager scope
	const font::manager font_manager;
	const sound::manager sound_manager;
#if !defined(__native_client__)
	const joystick::manager joystick_manager;
#endif 
	
	#if !TARGET_OS_IPHONE && !TARGET_IPHONE_SIMULATOR && !defined(__native_client__)
	std::cerr << "SDL_GetVideoSurface()" << std::endl;
	const SDL_Surface* fb = SDL_GetVideoSurface();
	if(fb == NULL) {
		return 0;
	}
	#endif

	graphics::texture::manager texture_manager;

#ifndef NO_EDITOR
	editor::manager editor_manager;
#endif

	variant preloads;
	loading_screen loader;
	try {
		sound::init_music(json::parse_from_file(module::map_file("data/music.cfg")));

		std::string filename = "data/fonts." + i18n::get_locale() + ".cfg";
		if (!sys::file_exists(filename))
			filename = "data/fonts.cfg";
		graphical_font::init(json::parse_from_file(module::map_file(filename)));

		preloads = json::parse_from_file(module::map_file("data/preload.cfg"));
		int preload_items = preloads["preload"].num_elements();
		loader.set_number_of_items(preload_items+7); // 7 is the number of items that will be loaded below
		custom_object::init();
		loader.draw_and_increment(_("Initializing custom object functions"));
		init_custom_object_functions(json::parse_from_file(module::map_file("data/functions.cfg")));
		loader.draw_and_increment(_("Initializing textures"));
		loader.load(preloads);
		loader.draw_and_increment(_("Initializing tiles"));
		tile_map::init(json::parse_from_file(module::map_file("data/tiles.cfg")));
		loader.draw_and_increment(_("Initializing GUI"));

		variant gui_node = json::parse_from_file(module::map_file(preferences::load_compiled() ? "data/compiled/gui.cfg" : "data/gui.cfg"));
		gui_section::init(gui_node);
		loader.draw_and_increment(_("Initializing GUI"));
		framed_gui_element::init(gui_node);

	} catch(const json::parse_error& e) {
		std::cerr << "ERROR PARSING: " << e.error_message() << "\n";
		return 0;
	}
	loader.draw(_("Loading level"));

#if defined(__native_client__)
	while(1) {
	}
#endif

	if(!skip_tests && !test::run_tests()) {
		return -1;
	}

	if(unit_tests_only) {
		return 0;
	}
#if defined(__APPLE__) && !(TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE)
	GLint swapInterval = 1;
	CGLSetParameter(CGLGetCurrentContext(), kCGLCPSwapInterval, &swapInterval);
#endif

	loader.finish_loading();
	//look to see if we got any quit events while loading.
	{
	SDL_Event event;
	while(SDL_PollEvent(&event)) {
		if(event.type == SDL_QUIT) {
			return 0;
		}
	}
	}

	formula_profiler::manager profiler(profile_output);

#ifdef USE_GLES2
	texture_frame_buffer::init(preferences::actual_screen_width(), preferences::actual_screen_height());
#else
	texture_frame_buffer::init();
#endif

	if(run_benchmarks) {
		if(benchmarks_list.empty() == false) {
			test::run_benchmarks(&benchmarks_list);
		} else {
			test::run_benchmarks();
		}
		return 0;
	} else if(utility_program.empty() == false) {
		test::run_utility(utility_program, util_args);
		return 0;
	}

	bool quit = false;
	bool of_initialized = false;

	while(!quit && !show_title_screen(level_cfg)) {
		boost::intrusive_ptr<level> lvl(load_level(level_cfg));
		
#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
		if (!of_initialized)
		{
			of_init();
			of_initialized = true;
		}
#endif

#if !defined(__native_client__)
		//see if we're loading a multiplayer level, in which case we
		//connect to the server.
		multiplayer::manager mp_manager(lvl->is_multiplayer());
		if(lvl->is_multiplayer()) {
			multiplayer::setup_networked_game(server);
		}

		if(lvl->is_multiplayer()) {
			last_draw_position() = screen_position();
			std::string level_cfg = "waiting-room.cfg";
			boost::intrusive_ptr<level> wait_lvl(load_level(level_cfg));
			wait_lvl->finish_loading();
			wait_lvl->set_multiplayer_slot(0);
			if(wait_lvl->player()) {
				wait_lvl->player()->set_current_level(level_cfg);
			}
			wait_lvl->set_as_current_level();

			level_runner runner(wait_lvl, level_cfg, orig_level_cfg);

			multiplayer::sync_start_time(*lvl, boost::bind(&level_runner::play_cycle, &runner));

			lvl->set_multiplayer_slot(multiplayer::slot());
		}
#endif

		last_draw_position() = screen_position();

		assert(lvl.get());
		if(!lvl->music().empty()) {
			sound::play_music(lvl->music());
		}

		if(lvl->player() && level_cfg != "autosave.cfg") {
			lvl->player()->set_current_level(level_cfg);
			lvl->player()->get_entity().save_game();
		}

		set_scene_title(lvl->title());

		try {
			quit = level_runner(lvl, level_cfg, orig_level_cfg).play_level();
			level_cfg = orig_level_cfg;
		} catch(multiplayer_exception&) {
		}
	}

	level::clear_current_level();

	} //end manager scope, make managers destruct before calling SDL_Quit
//	controls::debug_dump_controls();
#if defined(TARGET_PANDORA) || defined(TARGET_TEGRA)
    EGL_Destroy();
#endif

#if defined(USE_GLES2)
	wm.destroy_window();
#endif

	SDL_Quit();
	
	preferences::save_preferences();
	std::cerr << SDL_GetError() << "\n";
#if !defined(TARGET_OS_HARMATTAN) && !defined(TARGET_TEGRA) && !defined(TARGET_BLACKBERRY) && !defined(__ANDROID__) && !defined(USE_GLES2)
	std::cerr << gluErrorString(glGetError()) << "\n";
#endif
	return 0;
}
