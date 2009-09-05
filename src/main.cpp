#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <SDL/SDL.h>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <cstdio>

#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/scoped_ptr.hpp>

#include "asserts.hpp"
#include "character_type.hpp"
#include "controls.hpp"
#include "custom_object.hpp"
#include "custom_object_functions.hpp"
#include "custom_object_type.hpp"
#include "draw_scene.hpp"
#include "editor.hpp"
#include "filesystem.hpp"
#include "font.hpp"
#include "foreach.hpp"
#include "framed_gui_element.hpp"
#include "graphical_font.hpp"
#include "gui_section.hpp"
#include "inventory.hpp"
#include "item.hpp"
#include "joystick.hpp"
#include "key.hpp"
#include "level.hpp"
#include "level_object.hpp"
#include "level_runner.hpp"
#include "load_level.hpp"
#include "message_dialog.hpp"
#include "multiplayer.hpp"
#include "powerup.hpp"
#include "preferences.hpp"
#include "preprocessor.hpp"
#include "prop.hpp"
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

}

extern "C" int main(int argc, char** argv)
{
	#ifdef NO_STDERR
	std::freopen("/dev/null", "w", stderr);
	std::cerr.sync_with_stdio(true);
	#endif
	std::string level_cfg = "titlescreen.cfg";
	bool unit_tests_only = false, skip_tests = false;;
	bool run_benchmarks = false;
	std::string server = "wesnoth.org";
	for(int n = 1; n < argc; ++n) {
		std::string arg(argv[n]);
		if(arg == "--benchmarks") {
			run_benchmarks = true;
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

	if(!skip_tests && !test::run_tests()) {
		return -1;
	}

	if(unit_tests_only) {
		return 0;
	}

	if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_JOYSTICK) < 0) {
		std::cerr << "could not init SDL\n";
		return -1;
	}

	const stats::manager stats_manager;

	if(SDL_SetVideoMode(preferences::actual_screen_width(),preferences::actual_screen_height(),0,SDL_OPENGL|(preferences::fullscreen() ? SDL_FULLSCREEN : 0)) == NULL) {
		std::cerr << "could not set video mode\n";
		return -1;
	}

	std::cerr << "JOYSTICKS: " << SDL_NumJoysticks() << "\n";

	const load_level_manager load_manager;

	{ //manager scope
	const font::manager font_manager;
	const sound::manager sound_manager;
	const joystick::manager joystick_manager;

	const SDL_Surface* fb = SDL_GetVideoSurface();
	if(fb == NULL) {
		return 0;
	}

	sound::play("arrive.wav");

	graphics::texture::manager texture_manager;

	try {
		custom_object::init();
		init_custom_object_functions(wml::parse_wml_from_file("functions.cfg"));
		wml::schema::init(wml::parse_wml_from_file("schema.cfg"));
		character_type::init(wml::parse_wml_from_file("characters.cfg",
		                     wml::schema::get("characters")));
//		custom_object_type::init(wml::parse_wml_from_file("objects.cfg",
//								 wml::schema::get("objects")));
		item_type::init(wml::parse_wml_from_file("items.cfg",
		                wml::schema::get("items")));
		level_object::init(wml::parse_wml_from_file("tiles.cfg",
		                   wml::schema::get("tiles")));
		tile_map::init(wml::parse_wml_from_file("tiles.cfg",
		               wml::schema::get("tiles")));
		prop::init(wml::parse_wml_from_file("prop.cfg",
		           wml::schema::get("props")));
		powerup::init(wml::parse_wml_from_file("powerups.cfg",
		              wml::schema::get("powerups")));
		gui_section::init(wml::parse_wml_from_file("gui.cfg"));
		framed_gui_element::init(wml::parse_wml_from_file("gui.cfg"));
		graphical_font::init(wml::parse_wml_from_file("fonts.cfg"));
	} catch(const wml::parse_error& e) {
		return 0;
	}

	if(run_benchmarks) {
		test::run_benchmarks();
		return 0;
	}

	glShadeModel(GL_SMOOTH);
	glEnable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	GLenum glew_status = glewInit();
	ASSERT_EQ(glew_status, GLEW_OK);

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
	return 0;
}
