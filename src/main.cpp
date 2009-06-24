#include <GL/gl.h>
#include <GL/glu.h>
#include <SDL/SDL.h>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <cstdio>

#include <boost/lexical_cast.hpp>
#include <boost/scoped_ptr.hpp>

#include "character_type.hpp"
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
#include "load_level.hpp"
#include "message_dialog.hpp"
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
int screen_width = 800, screen_height = 600;
bool fullscreen = false;


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

void flip_scene(level& lvl, screen_position& screen_pos, bool flip_out) {
	if(!flip_out) {
		screen_pos.flip_rotate = 1000;
	}
	for(int n = 0; n != 20; ++n) {
		screen_pos.flip_rotate += 50 * (flip_out ? 1 : -1);
		lvl.process();
		draw_scene(lvl, screen_pos);

		SDL_GL_SwapBuffers();
		SDL_Delay(20);
	}

	screen_pos.flip_rotate = 0;
}

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

struct key_frames {
	int frame;
	std::string keys;
};

std::vector<key_frames> key_record;
int key_record_pos = 0;

void read_key_frames(const std::string& s) {
	std::vector<std::string> frames = util::split(s, ';');
	foreach(const std::string& f, frames) {
		std::vector<std::string> items = util::split(f, ':', 0);
		if(items.size() == 2) {
			key_frames frame;
			frame.frame = atoi(items.front().c_str());
			frame.keys = items.back();
			key_record.push_back(frame);
		}
	}
}

std::string write_key_frames() {
	std::ostringstream s;
	foreach(const key_frames& f, key_record) {
		s << f.frame << ":" << f.keys << ";";
	}

	return s.str();
}

bool play_level(boost::scoped_ptr<level>& lvl, std::string& level_cfg, bool record_replay)
{
	bool quit = false;
	boost::scoped_ptr<level> start_lvl;
	if(record_replay) {
		start_lvl.reset(load_level(level_cfg));
	}

	time_t current_second = time(NULL);
	int current_fps = 0, next_fps = 0;
	int current_delay = 0, next_delay = 0;
	int current_draw = 0, next_draw = 0;
	int current_process = 0, next_process = 0;

	CKey key;

	int cycle = 0;
	bool paused = false;
	bool done = false;
	while(!done) {
		const int desired_end_time = SDL_GetTicks() + 20;
		if(lvl->player() && lvl->player()->hitpoints() <= 0) {
			//record stats of the player's death
			lvl->player()->record_stats_movement();
			stats::record_event(lvl->id(), stats::record_ptr(new stats::die_record(lvl->player()->midpoint())));

			boost::intrusive_ptr<pc_character> save = lvl->player()->save_condition();
			if(!save) {
				return false;
			}
			preload_level(save->current_level());
			fade_scene(*lvl, last_draw_position());
			level* new_level = load_level(save->current_level());
			sound::play_music(new_level->music());
			set_scene_title(new_level->title());
			new_level->add_player(save);
			save->save_game();
			lvl.reset(new_level);
			last_draw_position() = screen_position();
		}

		const level::portal* portal = lvl->get_portal();
		if(portal) {
			//we might want to change the portal, so copy it and make it mutable.
			level::portal mutable_portal = *portal;
			portal = &mutable_portal;

			level_cfg = portal->level_dest;
			if(level_cfg.empty()) {
				//the portal is within the same level

				if(portal->dest_label.empty() == false) {
					const_entity_ptr dest_door = lvl->get_entity_by_label(portal->dest_label);
					if(dest_door) {
						mutable_portal.dest = point(dest_door->x(), dest_door->y());
						mutable_portal.dest_starting_pos = false;
					}
				}

				last_draw_position() = screen_position();
				character_ptr player = lvl->player();
				if(player) {
					player->set_pos(portal->dest);
				}
			} else {
				//the portal is to another level
				preload_level(level_cfg);

				const std::string transition = portal->transition;
				if(transition.empty() || transition == "fade") {
					fade_scene(*lvl, last_draw_position());
				} else if(transition == "flip") {
					flip_scene(*lvl, last_draw_position(), true);
				} else if(transition == "instant") {
					//do nothing.
				}

				level* new_level = load_level(level_cfg);

				if(portal->dest_label.empty() == false) {
					//the label of an object was specified as an entry point,
					//so set our position there.
					const_entity_ptr dest_door = new_level->get_entity_by_label(portal->dest_label);
					if(dest_door) {
						mutable_portal.dest = point(dest_door->x(), dest_door->y());
						mutable_portal.dest_starting_pos = false;
					}
				}

				sound::play_music(new_level->music());
				set_scene_title(new_level->title());
				point dest = portal->dest;
				if(portal->dest_str.empty() == false) {
					dest = new_level->get_dest_from_str(portal->dest_str);
				} else if(portal->dest_starting_pos) {
					character_ptr new_player = new_level->player();
					if(new_player) {
						dest = point(new_player->x(), new_player->y());
					}
				}

				character_ptr player = lvl->player();
				if(player && portal->saved_game == false) {
					std::cerr << "ADD PLAYER IN LEVEL\n";
					player->set_pos(dest);
					new_level->add_player(player);
					player->move_to_standing(*new_level);
				} else {
					std::cerr << "IS SAVED GAME\n";
					player = new_level->player();
				}

				lvl.reset(new_level);
				last_draw_position() = screen_position();

				if(transition == "flip") {
					flip_scene(*lvl, last_draw_position(), false);
				}
			}
		}

		joystick::update();
		//if we're in a replay any joystick motion will exit it.
		if(!record_replay && key_record.empty() == false) {
			if(joystick::left() || joystick::right() || joystick::up() || joystick::down() || joystick::button(0) || joystick::button(1) || joystick::button(2) || joystick::button(3)) {
				done = true;
			}
		}

		if(message_dialog::get() == NULL) {
			SDL_Event event;
			while(SDL_PollEvent(&event)) {
				switch(event.type) {
				case SDL_QUIT:
					done = true;
					quit = true;
					break;
				case SDL_VIDEORESIZE: {
					continue; //disabled.
					const SDL_ResizeEvent* const resize = reinterpret_cast<SDL_ResizeEvent*>(&event);
					screen_width = resize->w;
					screen_height = resize->h;
					if(screen_width > screen_height + screen_height/3) {
						screen_width = screen_height + screen_height/3;
					}

					if(screen_height > (screen_width*3)/4) {
						screen_height = (screen_width*3)/4;
					}
					SDL_SetVideoMode(screen_width,screen_height,0,SDL_OPENGL|(fullscreen ? SDL_FULLSCREEN : 0));

				}
				case SDL_KEYDOWN: {
					//if we're in a replay any keypress will exit it.
					if(!record_replay && key_record.empty() == false) {
						done = true;
						break;
					}

					const SDLMod mod = SDL_GetModState();
					const SDLKey key = event.key.keysym.sym;
					if(key == SDLK_ESCAPE) {
						//record a quit event in stats
						if(lvl->player()) {
							lvl->player()->record_stats_movement();
							stats::record_event(lvl->id(), stats::record_ptr(new stats::quit_record(lvl->player()->midpoint())));
						}

						done = true;
						quit = true;
						break;
					} else if(key == SDLK_e && (mod&KMOD_CTRL)) {
						editor::edit(lvl->id().c_str(), last_draw_position().x/100, last_draw_position().y/100);
						lvl.reset(load_level(editor::last_edited_level().c_str()));
					} else if(key == SDLK_s && (mod&KMOD_CTRL)) {
						std::string data;
						
						wml::node_ptr lvl_node = wml::deep_copy(lvl->write());
						if(record_replay) {
							lvl_node = wml::deep_copy(start_lvl->write());
							lvl_node->set_attr("replay_data", write_key_frames());
						}
						wml::write(lvl_node, data);
						sys::write_file("save.cfg", data);
					} else if(key == SDLK_w && (mod&KMOD_CTRL)) {
						//warp to another level.
						std::vector<std::string> levels = get_known_levels();
						assert(!levels.empty());
						int index = std::find(levels.begin(), levels.end(), lvl->id()) - levels.begin();
						index = (index+1)%levels.size();
						level* new_level = load_level(levels[index]);
						sound::play_music(new_level->music());
						set_scene_title(new_level->title());
						lvl.reset(new_level);
					} else if(key == SDLK_l && (mod&KMOD_CTRL)) {
						preferences::set_use_pretty_scaling(!preferences::use_pretty_scaling());
						graphics::surface_cache::clear();
						graphics::texture::clear_cache();
					} else if(key == SDLK_i && lvl->player()) {
						show_inventory(*lvl->player());
					} else if(key == SDLK_m && mod & KMOD_CTRL) {
						sound::mute(!sound::muted()); //toggle sound
					} else if(key == SDLK_p && mod & KMOD_CTRL) {
						paused = !paused;
					} else if(key == SDLK_p && mod & KMOD_ALT) {
						preferences::set_use_pretty_scaling(!preferences::use_pretty_scaling());
						graphics::texture::clear_textures();
					} else if(key == SDLK_f && mod & KMOD_CTRL) {
						fullscreen = !fullscreen;
						SDL_SetVideoMode(screen_width,screen_height,0,SDL_OPENGL|(fullscreen ? SDL_FULLSCREEN : 0));
					}
					break;
				}
				default:
					break;
				}
			}
		}

		if(record_replay) {
			std::string data;
			key.Write(&data);
			if(key_record.empty() || key_record.back().keys != data) {
				key_frames f;
				f.frame = cycle;
				f.keys = data;
				key_record.push_back(f);
			}
		}

		if(!record_replay && key_record.empty() == false) {
			if(key_record_pos < key_record.size() && key_record[key_record_pos].frame == cycle) {
				if(lvl->player()) {
					lvl->player()->set_key_state(key_record[key_record_pos].keys);
				}
				++key_record_pos;

				std::cerr << "SHOW_FRAME: " << key_record_pos << "/" << key_record.size() << "\n";

				if(key_record_pos == key_record.size()) {
					fade_scene(*lvl, last_draw_position());
					return false;
				}
			}
		}

		if(message_dialog::get()) {
			message_dialog::get()->process();
		} else {
			if (!paused) {
				const int start_process = SDL_GetTicks();
				lvl->process();
				next_process += (SDL_GetTicks() - start_process);
			}
		}

		if(lvl->end_game()) {
			fade_scene(*lvl, last_draw_position());
			show_end_game();
			done = true;
			break;
		}

		const int start_draw = SDL_GetTicks();
		draw_scene(*lvl, last_draw_position());
		next_draw += (SDL_GetTicks() - start_draw);

		draw_fps(*lvl, current_fps, current_delay, current_draw, current_process);
		
		SDL_GL_SwapBuffers();
		++next_fps;

		const time_t this_second = time(NULL);
		if(this_second != current_second) {
			current_second = this_second;
			current_fps = next_fps;
			current_delay = next_delay;
			current_draw = next_draw;
			current_process = next_process;
			next_fps = 0;
			next_delay = 0;
			next_draw = 0;
			next_process = 0;
		}

		const int wait_time = std::max<int>(1, desired_end_time - SDL_GetTicks());
		next_delay += wait_time;
		SDL_Delay(wait_time);
		std::cerr << "delay: " << wait_time << "\n";

		if (!paused) ++cycle;
	}

	return quit;
}

}

extern "C" int main(int argc, char** argv)
{
	#ifdef NO_STDERR
	std::freopen("/dev/null", "w", stderr);
	std::cerr.sync_with_stdio(true);
	#endif
	bool record_replay = false;
	std::string level_cfg = "titlescreen.cfg";
	bool unit_tests_only = false, skip_tests = false;;
	bool run_benchmarks = false;
	for(int n = 1; n < argc; ++n) {
		std::string arg(argv[n]);
		if(arg == "--benchmarks") {
			run_benchmarks = true;
		} else if(arg == "--tests") {
			unit_tests_only = true;
		} else if(arg == "--notests") {
			skip_tests = true;
		} else if(arg == "--fullscreen") {
			fullscreen = true;
		} else if(arg == "--width") {
			std::string w(argv[++n]);
			screen_width = boost::lexical_cast<int>(w);
		} else if(arg == "--height" && n+1 < argc) {
			std::string h(argv[++n]);
			screen_height = boost::lexical_cast<int>(h);
		} else if(arg == "--level" && n+1 < argc) {
			level_cfg = argv[++n];
		} else if(arg == "--record_replay") {
			record_replay = true;
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

	if(run_benchmarks) {
		test::run_benchmarks();
		return 0;
	}

	if(unit_tests_only) {
		return 0;
	}

	if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_JOYSTICK) < 0) {
		std::cerr << "could not init SDL\n";
		return -1;
	}

	const stats::manager stats_manager;

	if(SDL_SetVideoMode(screen_width,screen_height,0,SDL_OPENGL|(fullscreen ? SDL_FULLSCREEN : 0)) == NULL) {
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
		init_custom_object_functions(wml::parse_wml(sys::read_file("functions.cfg")));
		wml::schema::init(wml::parse_wml(sys::read_file("schema.cfg")));
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

	glShadeModel(GL_SMOOTH);
	glEnable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	bool quit = false;
	const std::string orig_level_cfg = level_cfg;

	while(!quit && !show_title_screen(level_cfg)) {
		last_draw_position() = screen_position();

		boost::scoped_ptr<level> lvl(load_level(level_cfg));
		assert(lvl.get());
		sound::play_music(lvl->music());
		if(lvl->player()) {
			lvl->player()->set_current_level(level_cfg);
			lvl->player()->save_game();
		}
		set_scene_title(lvl->title());

		if(lvl->replay_data().empty() == false) {
			read_key_frames(lvl->replay_data());
			key_record_pos = 0;
		}

		quit = play_level(lvl, level_cfg, record_replay);
		level_cfg = orig_level_cfg;

		key_record.clear();
		key_record_pos = 0;
	}

	} //end manager scope, make managers destruct before calling SDL_Quit
	std::cerr << "quitting...\n";
	SDL_Quit();
	std::cerr << "quit called...\n";
	return 0;
}
