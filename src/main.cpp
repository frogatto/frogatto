#include <SDL.h>
#ifndef SDL_VIDEO_OPENGL_ES
#include <GL/glew.h>
#endif
#if defined(TARGET_OS_HARMATTAN) || defined(TARGET_PANDORA) || defined(TARGET_TEGRA)
#include <GLES/gl.h>
#ifdef TARGET_PANDORA
#include <GLES/glues.h>
#endif
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif
#if defined(__APPLE__) && !(TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE)
#include <OpenGL/OpenGL.h>
#endif
#include <algorithm>
#include <cmath>
#include <iostream>
#include <cstdio>

#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/scoped_ptr.hpp>

#include "asserts.hpp"
#include "controls.hpp"
#include "custom_object.hpp"
#include "custom_object_functions.hpp"
#include "custom_object_type.hpp"
#include "draw_scene.hpp"
#ifndef NO_EDITOR
#include "editor.hpp"
#endif
#include "filesystem.hpp"
#include "font.hpp"
#include "foreach.hpp"
#include "formula_profiler.hpp"
#include "framed_gui_element.hpp"
#include "graphical_font.hpp"
#include "gui_section.hpp"
#include "i18n.hpp"
#include "inventory.hpp"
#include "iphone_device_info.h"
#include "of_bridge.h"
#include "joystick.hpp"
#include "key.hpp"
#include "level.hpp"
#include "level_object.hpp"
#include "level_runner.hpp"
#include "load_level.hpp"
#include "loading_screen.hpp"
#include "message_dialog.hpp"
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
#include "wml_node.hpp"
#include "wml_parser.hpp"
#include "wml_schema.hpp"
#include "wml_utils.hpp"
#include "wml_writer.hpp"

#if defined(TARGET_PANDORA) | defined(TARGET_TEGRA)
#include "eglport.h"
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
"      --native			        one pixel in-game equals one pixel on monitor." <<
"      --[no-]music             enables/disables game music\n" <<
"      --relay                  use the server as a relay in multiplayer rather\n" <<
"                                 than trying to initiate direct connections\n" <<
"      --[no-]resizable         allows/disallows to resize the game window\n" <<
"      --scale                  enables an experimental pixel art interpolation\n" <<
"                                 algorithm for scaling the game graphics (some\n" <<
"                                 issues with this still have to be solved)\n" <<
"      --[no-]send-stats        enables/disables sending game statistics over the\n" <<
"                                 network\n" <<
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
//"      --profile                FIXME\n" <<
//"      --profile=FILE           FIXME\n" <<
"      --show-hitboxes          turns on the display of object hitboxes\n" <<
"      --show-controls          turns on the display of iPhone control hitboxes\n" <<
"      --simipad                changes various options to emulate an iPad\n" <<
"                                 environment\n" <<
"      --simiphone              changes various options to emulate an iPhone\n" <<
"                                 environment\n" <<
"      --tests                  runs the game's unit tests and exits\n" <<
"      --no-tests               skips the execution of unit tests on startup\n"
"      --utility=NAME           runs the specified UTILITY( NAME ) code block,\n" <<
"                                 such as compile_levels or object_compiler,\n" <<
"                                 with the specified arguments\n"
	;
}

}

extern "C" int main(int argc, char** argv)
{
#if defined(TARGET_PANDORA)
    EGL_Open();
#endif

	if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_JOYSTICK) < 0) {
		std::cerr << "could not init SDL\n";
		return -1;
	}

	i18n::init ();

//	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 1);

	#ifdef NO_STDERR
	std::freopen("/dev/null", "w", stderr);
	std::cerr.sync_with_stdio(true);
	#endif

	std::cerr << "Frogatto engine version " << preferences::version() << "\n";

	std::string level_cfg = "titlescreen.cfg";
	bool unit_tests_only = false, skip_tests = false;
	bool run_benchmarks = false;
	std::vector<std::string> benchmarks_list;
	std::string utility_program;
	std::vector<std::string> util_args;
	std::string server = "wesnoth.org";

	const char* profile_output = NULL;
	std::string profile_output_buf;

	std::string orig_level_cfg = level_cfg;
	std::string override_level_cfg = "";

	preferences::load_preferences();

	for(int n = 1; n < argc; ++n) {
		const std::string arg(argv[n]);
		std::string arg_name, arg_value;
		std::string::const_iterator equal = std::find(arg.begin(), arg.end(), '=');
		if(equal != arg.end()) {
			arg_name = std::string(arg.begin(), equal);
			arg_value = std::string(equal+1, arg.end());
		}
		
		if(arg_name == "--profile" || arg == "--profile") {
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
		} else if(arg == "--no-compiled") {
			preferences::set_load_compiled(false);
#if defined(TARGET_PANDORA)
		} else if(arg == "--no-fbo") {
			preferences::set_fbo(false);
		} else if(arg == "--no-bequ") {
			preferences::set_bequ(false);
#endif
		} else if(arg == "--help" || arg == "-h") {
			print_help(std::string(argv[0]));
			return 0;
		} else {
			const bool res = preferences::parse_arg(argv[n]);
			if(!res) {
				std::cerr << "unrecognized arg: '" << arg << "'\n";
				return -1;
			}
		}
	}

	preferences::expand_data_paths();

	std::cerr << "Preferences dir: " << preferences::user_data_path() << '\n';

	//make sure that the user data path exists.
	if(!preferences::setup_preferences_dir()) {
		std::cerr << "cannot create preferences dir!\n";
	}

	std::cerr << "\n";

#ifdef TARGET_OS_IPHONE
	//on the iPhone, try to restore the auto-save if it exists
	if(sys::file_exists(preferences::auto_save_file_path()) && sys::read_file(std::string(preferences::auto_save_file_path()) + ".stat") == "1") {
		level_cfg = "autosave.cfg";
		sys::write_file(std::string(preferences::auto_save_file_path()) + ".stat", "0");

	}
#endif

	if(override_level_cfg.empty() != true) {
		level_cfg = override_level_cfg;
		orig_level_cfg = level_cfg;
	}

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
#else
#if defined(TARGET_TEGRA)
	//if (SDL_SetVideoMode(preferences::actual_screen_width(),preferences::actual_screen_height(),0,preferences::resizable() ? SDL_RESIZABLE : 0|preferences::fullscreen() ? SDL_FULLSCREEN : 0) == NULL) {
	if (SDL_SetVideoMode(preferences::actual_screen_width(),preferences::actual_screen_height(),0,SDL_FULLSCREEN) == NULL) {
		std::cerr << "could not set video mode\n";
		return -1;
	}
    EGL_Init();
    preferences::init_oes();
#else
	if (SDL_SetVideoMode(preferences::actual_screen_width(),preferences::actual_screen_height(),0,SDL_OPENGL|(preferences::resizable() ? SDL_RESIZABLE : 0)|(preferences::fullscreen() ? SDL_FULLSCREEN : 0)) == NULL) {
		std::cerr << "could not set video mode\n";
		return -1;
	}
#endif
#endif
#endif

#endif

//	srand(time(NULL));

	const stats::manager stats_manager;

	std::cerr
		<< "\n"
		<< "OpenGL vendor: " << reinterpret_cast<const char *>(glGetString(GL_VENDOR)) << "\n"
		<< "OpenGL version: " << reinterpret_cast<const char *>(glGetString(GL_VERSION)) << "\n"
		<< "OpenGL extensions: " << reinterpret_cast<const char *>(glGetString(GL_EXTENSIONS)) << "\n"
		<< "\n";
	glShadeModel(GL_SMOOTH);
	glEnable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	SDL_WM_SetCaption("Frogatto", "Frogatto");

	std::cerr << "JOYSTICKS: " << SDL_NumJoysticks() << "\n";

	const load_level_manager load_manager;

	{ //manager scope
	const font::manager font_manager;
	const sound::manager sound_manager;
	const joystick::manager joystick_manager;
		
	#if !TARGET_OS_IPHONE && !TARGET_IPHONE_SIMULATOR
	const SDL_Surface* fb = SDL_GetVideoSurface();
	if(fb == NULL) {
		return 0;
	}
	#endif

	graphics::texture::manager texture_manager;

	wml::const_node_ptr preloads;
	loading_screen loader;
	
	try {
		wml::schema::init(wml::parse_wml_from_file("data/schema.cfg"));

		std::string filename = "data/fonts." + i18n::get_locale() + ".cfg";
		if (!sys::file_exists(filename))
			filename = "data/fonts.cfg";
		graphical_font::init(wml::parse_wml_from_file(filename));

		preloads = wml::parse_wml_from_file("data/preload.cfg");
		int preload_items = std::distance(preloads->begin_child("preload"), preloads->end_child("preload"));
		loader.set_number_of_items(preload_items+7); // 7 is the number of items that will be loaded below
		loader.load(preloads);
		loader.draw_and_increment(_("Initializing custom object"));
		custom_object::init();
		loader.draw_and_increment(_("Initializing custom object functions"));
		init_custom_object_functions(wml::parse_wml_from_file("data/functions.cfg"));
		loader.draw_and_increment(_("Initializing tiles"));
		tile_map::init(wml::parse_wml_from_file("data/tiles.cfg",
		               wml::schema::get("tiles")));
		loader.draw_and_increment(_("Initializing GUI"));

		wml::const_node_ptr gui_node = wml::parse_wml_from_file(preferences::load_compiled() ? "data/compiled/gui.cfg" : "data/gui.cfg");
		gui_section::init(gui_node);
		loader.draw_and_increment(_("Initializing GUI"));
		framed_gui_element::init(gui_node);
	} catch(const wml::parse_error& e) {
		return 0;
	}
	
	loader.draw(_("Loading level"));

	if(!skip_tests && !test::run_tests()) {
		return -1;
	}

	if(unit_tests_only) {
		return 0;
	}

#if defined(__APPLE__) && !(TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE)
	long swapInterval = 1;
	CGLSetParameter(CGLGetCurrentContext(), kCGLCPSwapInterval, &swapInterval);
#endif

#ifndef SDL_VIDEO_OPENGL_ES
	GLenum glew_status = glewInit();
	ASSERT_EQ(glew_status, GLEW_OK);
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
	texture_frame_buffer::init();

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

	} //end manager scope, make managers destruct before calling SDL_Quit

//	controls::debug_dump_controls();
#if defined(TARGET_PANDORA) || defined(TARGET_TEGRA)
    EGL_Destroy();
#endif

	SDL_Quit();
	
	preferences::save_preferences();
	std::cerr << SDL_GetError() << "\n";
#if !defined(TARGET_OS_HARMATTAN) && !defined(TARGET_TEGRA)
	std::cerr << gluErrorString(glGetError()) << "\n";
#endif
	return 0;
}
