#include <GL/gl.h>
#include <GL/glu.h>
#include <SDL/SDL.h>
#include <algorithm>
#include <cmath>
#include <iostream>

#include <boost/lexical_cast.hpp>
#include <boost/scoped_ptr.hpp>

#include "character_type.hpp"
#include "custom_object_type.hpp"
#include "draw_scene.hpp"
#include "filesystem.hpp"
#include "font.hpp"
#include "item.hpp"
#include "joystick.hpp"
#include "key.hpp"
#include "level.hpp"
#include "level_object.hpp"
#include "message_dialog.hpp"
#include "raster.hpp"
#include "sound.hpp"
#include "texture.hpp"
#include "tile_map.hpp"
#include "wml_node.hpp"
#include "wml_parser.hpp"

void edit_level(const char* level_cfg);

namespace {

void fade_scene(level& lvl, screen_position& screen_pos) {
	for(int n = 0; n < 255; n += 20) {
		lvl.process();
		draw_scene(lvl, screen_pos);
		const SDL_Rect r = {0, 0, graphics::screen_width(), graphics::screen_height()};
		const SDL_Color c = {0,0,0,0};
		graphics::draw_rect(r, c, n);
		SDL_GL_SwapBuffers();
		SDL_Delay(20);		
	}

	const SDL_Rect r = {0, 0, graphics::screen_width(), graphics::screen_height()};
	const SDL_Color c = {0,0,0,0};
	graphics::draw_rect(r, c, 255);
	SDL_GL_SwapBuffers();
}

bool show_title_screen()
{
	graphics::texture img(graphics::texture::get("titlescreen.png"));

	for(;;) {
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

				return false;
			}
		}

		SDL_Delay(50);
	}

	return true;
}

void show_end_game()
{
	const std::string msg = "to be continued...";
	graphics::texture t(font::render_text(msg, graphics::color_white(), 48));
	const int xpos = graphics::screen_width()/2 - t.width()/2;
	const int ypos = graphics::screen_height()/2 - t.height()/2;
	for(int n = 0; n <= msg.size(); ++n) {
		const GLfloat percent = GLfloat(n)/GLfloat(msg.size());
		SDL_Rect rect = {0, 0, graphics::screen_width(), graphics::screen_height()};
		graphics::draw_rect(rect, graphics::color_black());
		graphics::blit_texture(t, xpos, ypos, t.width()*percent, t.height(), 0.0,
						       0.0, 0.0, percent, 1.0);
		SDL_GL_SwapBuffers();
		SDL_Delay(40);
	}

	bool done = false;
	while(!done) {
		SDL_Event event;
		while(SDL_PollEvent(&event)) {
			switch(event.type) {
			case SDL_QUIT:
			case SDL_KEYDOWN:
				done = true;
				break;
			}
		}
		joystick::update();
		for(int n = 0; n != 6; ++n) {
			if(joystick::button(n)) {
				done = true;
			}
		}
	}
}

}

extern "C" int main(int argc, char** argv)
{
	bool fullscreen = false;
	int width = 800, height = 600;
	std::string level_cfg = "level0.cfg";
	for(int n = 1; n < argc; ++n) {
		std::string arg(argv[n]);
		if(arg == "--fullscreen") {
			fullscreen = true;
		} else if(arg == "--width") {
			std::string w(argv[++n]);
			width = boost::lexical_cast<int>(w);
		} else if(arg == "--height" && n+1 < argc) {
			std::string h(argv[++n]);
			height = boost::lexical_cast<int>(h);
		} else if(arg == "--level" && n+1 < argc) {
			level_cfg = argv[++n];
		} else {
			std::cerr << "unrecognized arg: '" << arg << "'\n";
			return 0;
		}
	}

	if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_JOYSTICK) < 0) {
		std::cerr << "could not init SDL\n";
		return -1;
	}

	if(SDL_SetVideoMode(width,height,0,SDL_OPENGL|(fullscreen ? SDL_FULLSCREEN : 0)) == NULL) {
		std::cerr << "could not set video mode\n";
		return -1;
	}

	std::cerr << "JOYSTICKS: " << SDL_NumJoysticks() << "\n";

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

	character_type::init(wml::parse_wml(sys::read_file("characters.cfg")));
	custom_object_type::init(wml::parse_wml(sys::read_file("objects.cfg")));
	item_type::init(wml::parse_wml(sys::read_file("items.cfg")));
	level_object::init(wml::parse_wml(sys::read_file("tiles.cfg")));
	tile_map::init(wml::parse_wml(sys::read_file("tiles.cfg")));
	boost::scoped_ptr<level> lvl(new level(level_cfg));
	if(lvl->player()) {
		lvl->player()->set_current_level(level_cfg);
		lvl->player()->save_game();
	}
	set_scene_title(lvl->title());

	glEnable(GL_SMOOTH);
	glEnable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	CKey key;

	std::cerr << "FONT SIZE: " << font::char_width(14) << "," << font::char_height(14) << "\n";

	bool done = show_title_screen();
	while(!done) {
		const int desired_end_time = SDL_GetTicks() + 20;
		if(lvl->player() && lvl->player()->hitpoints() <= 0) {
			fade_scene(*lvl, last_draw_position());
			boost::intrusive_ptr<pc_character> save = lvl->player()->save_condition();
			if(!save) {
				return 0;
			}
			level* new_level = new level(save->current_level());
			set_scene_title(new_level->title());
			new_level->add_player(save);
			save->save_game();
			lvl.reset(new_level);
			last_draw_position() = screen_position();
		}

		const level::portal* portal = lvl->get_portal();
		if(portal) {
			fade_scene(*lvl, last_draw_position());
			level_cfg = portal->level_dest;
			level* new_level = new level(level_cfg);
			set_scene_title(new_level->title());
			point dest = portal->dest;
			if(portal->dest_starting_pos) {
				character_ptr new_player = new_level->player();
				if(new_player) {
					dest = point(new_player->x(), new_player->y());
				}
			}

			character_ptr player = lvl->player();
			if(player) {
				player->set_pos(dest);
				new_level->add_player(player);
				player->move_to_standing(*new_level);
			}

			lvl.reset(new_level);
			last_draw_position() = screen_position();
		}

		joystick::update();

		if(message_dialog::get() == NULL) {
			SDL_Event event;
			while(SDL_PollEvent(&event)) {
				switch(event.type) {
				case SDL_QUIT:
					done = true;
					break;
				case SDL_KEYDOWN: {
					const SDLMod mod = SDL_GetModState();
					if(event.key.keysym.sym == SDLK_ESCAPE) {
						done = true;
						break;
					} else if(event.key.keysym.sym == SDLK_e && (mod&KMOD_CTRL)) {
						std::cerr << "editor...\n";
						edit_level(level_cfg.c_str());
						lvl.reset(new level(level_cfg));
					}
					break;
				}
				default:
					break;
				}
			}
		}

		if(message_dialog::get()) {
			message_dialog::get()->process();
		} else {
			lvl->process();
		}

		if(lvl->end_game()) {
			fade_scene(*lvl, last_draw_position());
			show_end_game();
			done = true;
			break;
		}

		draw_scene(*lvl, last_draw_position());
		SDL_GL_SwapBuffers();

		const int wait_time = std::max<int>(1, desired_end_time - SDL_GetTicks());
		std::cerr << "wait: " << wait_time << "\n";
		SDL_Delay(wait_time);
	}

	} //end manager scope, make managers destruct before calling SDL_Quit
	std::cerr << "quitting...\n";
	SDL_Quit();
	std::cerr << "quit called...\n";
	return 0;
}
