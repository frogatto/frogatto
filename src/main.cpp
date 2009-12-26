#include <SDL.h>
#ifndef SDL_VIDEO_OPENGL_ES
#include <GL/glew.h>
#endif
#include <GL/gl.h>
#include <GL/glu.h>
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
#include "framed_gui_element.hpp"
#include "graphical_font.hpp"
#include "gui_section.hpp"
#include "inventory.hpp"
#include "joystick.hpp"
#include "key.hpp"
#include "level.hpp"
#include "level_object.hpp"
#include "level_runner.hpp"
#include "load_level.hpp"
#include "message_dialog.hpp"
#include "multiplayer.hpp"
#include "player_info.hpp"
#include "powerup.hpp"
#include "preferences.hpp"
#include "preprocessor.hpp"
#include "raster.hpp"
#include "sound.hpp"
#include "stats.hpp"
#include "string_utils.hpp"
#include "surface_cache.hpp"
#include "texture.hpp"
#include "tile_map.hpp"
#include "unit_test.hpp"
#include "wml_node.hpp"
#include "wml_parser.hpp"
#include "wml_schema.hpp"
#include "wml_utils.hpp"
#include "wml_writer.hpp"

namespace {

bool show_title_screen(std::string& level_cfg)
{
	//currently the titlescreen is disabled.
	return false;

	preload_level(level_cfg);

	const int CyclesUntilPreloadReplay = 15*20;
	const int CyclesUntilShowReplay = 18*20;

	graphics::texture img(graphics::texture::get("titlescreen.png"));

	for(int cycle = 0; ; ++cycle) {
		if(cycle == CyclesUntilPreloadReplay) {
			preload_level("replay.cfg");
		} else if(cycle == CyclesUntilShowReplay) {
			level_cfg = "replay.cfg";
			return false;
		}

		graphics::prepare_raster();
		graphics::blit_texture(img, 0, 0, graphics::screen_width(), graphics::screen_height());
		
		SDL_GL_SwapBuffers();
		joystick::update();
		for(int n = 0; n != 6; ++n) {
			if(joystick::button(n)) {
				return false;
			}
		}
		SDL_Event event;
		while(SDL_PollEvent(&event)) {
			switch(event.type) {
			case SDL_QUIT:
				return true;
			case SDL_KEYDOWN:
				if(event.key.keysym.sym == SDLK_ESCAPE) {
					return true;
				}
				
				//These ifs make the game not start if you're trying to do a command, on Mac
#if SDL_MAJOR_VERSION == 1 && SDL_MINOR_VERSION == 3
#define SDLK_LMETA SDLK_LGUI
#define SDLK_RMETA SDLK_RGUI
#endif
				if(event.key.keysym.sym != SDLK_LMETA && !(event.key.keysym.mod & KMOD_LMETA) &&
					event.key.keysym.sym != SDLK_RMETA && !(event.key.keysym.mod & KMOD_RMETA)){
					return false;
				}
			}
		}

		SDL_Delay(50);
	}

	return true;
}

void iphone_test ()
{
	graphics::texture img(graphics::texture::get("titlescreen.png"));
	bool done = false;
	while (!done)
	{
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_QUIT || event.type == SDL_MOUSEBUTTONDOWN)
				done = true;
		}
		graphics::prepare_raster();
		graphics::blit_texture(img, 0, 0, 200, 100);
		std::cerr << gluErrorString(glGetError()) << "~1\n";
		glDisable(GL_TEXTURE_2D);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		SDL_Rect rect = {10, 10, 50, 50};
		glColor4f(1.0, 0.5, 0.0, 1.0);
		GLfloat vertices[] = {
			rect.x, rect.y,
			rect.x+rect.w, rect.y,
			rect.x, rect.y+rect.h,
			rect.x+rect.w, rect.y+rect.h
		};
		glVertexPointer(2, GL_FLOAT, 0, vertices);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glEnable(GL_TEXTURE_2D);
		SDL_GL_SwapBuffers();
		SDL_Delay(500);
	}
}

}

extern "C" int main(int argc, char** argv)
{
	#ifdef NO_STDERR
	std::freopen("/dev/null", "w", stderr);
	std::cerr.sync_with_stdio(true);
	#endif
	std::string level_cfg = "titlescreen.cfg";
	bool unit_tests_only = false, skip_tests = false;
	bool run_benchmarks = false;
	std::vector<std::string> benchmarks_list;
	std::string server = "wesnoth.org";
	for(int n = 1; n < argc; ++n) {
		const std::string arg(argv[n]);
		std::string arg_name, arg_value;
		std::string::const_iterator equal = std::find(arg.begin(), arg.end(), '=');
		if(equal != arg.end()) {
			arg_name = std::string(arg.begin(), equal);
			arg_value = std::string(equal+1, arg.end());
		}
		
		if(arg == "--benchmarks") {
			run_benchmarks = true;
		} else if(arg_name == "--benchmarks") {
			run_benchmarks = true;
			benchmarks_list = util::split(arg_value);
		} else if(arg == "--tests") {
			unit_tests_only = true;
		} else if(arg == "--notests") {
			skip_tests = true;
		} else if(arg == "--width" && n+1 < argc) {
			std::string w(argv[++n]);
			preferences::set_actual_screen_width(boost::lexical_cast<int>(w));
		} else if(arg == "--height" && n+1 < argc) {
			std::string h(argv[++n]);
			preferences::set_actual_screen_height(boost::lexical_cast<int>(h));
		} else if(arg == "--level" && n+1 < argc) {
			level_cfg = argv[++n];
		} else if(arg == "--host" && n+1 < argc) {
			server = argv[++n];
		} else {
			const bool res = preferences::parse_arg(argv[n]);
			if(!res) {
				std::cerr << "unrecognized arg: '" << arg << "'\n";
				return 0;
			}
		}
	}

	srand(time(NULL));

	if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_JOYSTICK) < 0) {
		std::cerr << "could not init SDL\n";
		return -1;
	}

	const stats::manager stats_manager;

	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 1);
	
#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
	SDL_WindowID windowID = SDL_CreateWindow (NULL, 0, 0, preferences::actual_screen_width(), preferences::actual_screen_height(),
		SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN |
		SDL_WINDOW_BORDERLESS);
	if (windowID == 0) { 
		std::cerr << "Could not create window: " << SDL_GetError() << "\n"; 
		return -1;
	}
	
	if (SDL_GL_CreateContext(windowID) == 0) {
		std::cerr << "Could not create GL context: " << SDL_GetError() << "\n";
		return -1;
	}
	
#else
	if (SDL_SetVideoMode(preferences::actual_screen_width(),preferences::actual_screen_height(),0,SDL_OPENGL|(preferences::fullscreen() ? SDL_FULLSCREEN : 0)) == NULL) {
		std::cerr << "could not set video mode\n";
		return -1;
	}
#endif

	

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

	sound::play("arrive.wav");

	graphics::texture::manager texture_manager;

	try {
		custom_object::init();
		init_custom_object_functions(wml::parse_wml_from_file("functions.cfg"));
		wml::schema::init(wml::parse_wml_from_file("schema.cfg"));
		tile_map::init(wml::parse_wml_from_file("tiles.cfg",
		               wml::schema::get("tiles")));
		powerup::init(wml::parse_wml_from_file("powerups.cfg",
		              wml::schema::get("powerups")));
		gui_section::init(wml::parse_wml_from_file("gui.cfg"));
		framed_gui_element::init(wml::parse_wml_from_file("gui.cfg"));
		graphical_font::init(wml::parse_wml_from_file("fonts.cfg"));
	} catch(const wml::parse_error& e) {
		return 0;
	}

	if(!skip_tests && !test::run_tests()) {
		return -1;
	}

	if(unit_tests_only) {
		return 0;
	}

	if(run_benchmarks) {
		if(benchmarks_list.empty() == false) {
			test::run_benchmarks(&benchmarks_list);
		} else {
			test::run_benchmarks();
		}
		return 0;
	}

	glShadeModel(GL_SMOOTH);
	glEnable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		
#if TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE
	//iphone_test();
#endif

#ifndef SDL_VIDEO_OPENGL_ES
	GLenum glew_status = glewInit();
	ASSERT_EQ(glew_status, GLEW_OK);
#endif

	bool quit = false;
	const std::string orig_level_cfg = level_cfg;

	while(!quit && !show_title_screen(level_cfg)) {
		boost::scoped_ptr<level> lvl(load_level(level_cfg));

		//see if we're loading a multiplayer level, in which case we
		//connect to the server.
		multiplayer::manager mp_manager(lvl->is_multiplayer());
		if(lvl->is_multiplayer()) {
			multiplayer::setup_networked_game(server);
		}

		if(lvl->is_multiplayer()) {
			last_draw_position() = screen_position();
			std::string level_cfg = "waiting-room.cfg";
			boost::scoped_ptr<level> wait_lvl(load_level(level_cfg));
			wait_lvl->set_multiplayer_slot(0);
			std::cerr << "HAS PLAYER : " << (wait_lvl->player() ? "YES" : "NO") << "\n";
			if(wait_lvl->player()) {
				wait_lvl->player()->set_current_level(level_cfg);
			}
			level_runner runner(wait_lvl, level_cfg);

			multiplayer::sync_start_time(*lvl, boost::bind(&level_runner::play_cycle, &runner));

			lvl->set_multiplayer_slot(multiplayer::slot());
		}

		last_draw_position() = screen_position();

		assert(lvl.get());
		sound::play_music(lvl->music());
		if(lvl->player()) {
			lvl->player()->set_current_level(level_cfg);
			lvl->player()->get_entity().save_game();
		}

		set_scene_title(lvl->title());

		try {
			quit = level_runner(lvl, level_cfg).play_level();
			level_cfg = orig_level_cfg;
		} catch(multiplayer_exception&) {
		}
	}

	} //end manager scope, make managers destruct before calling SDL_Quit

//	controls::debug_dump_controls();
	std::cerr << "quitting...\n";
	SDL_Quit();
	std::cerr << "quit called...\n";
	std::cerr << SDL_GetError() << "\n";
	std::cerr << gluErrorString(glGetError()) << "\n";
	return 0;
}
