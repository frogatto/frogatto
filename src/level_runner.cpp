#include <math.h>

#include <boost/function.hpp>

#include "SDL.h"

#include "controls.hpp"
#include "draw_scene.hpp"
#include "editor.hpp"
#include "filesystem.hpp"
#include "font.hpp"
#include "foreach.hpp"
#include "inventory.hpp"
#include "joystick.hpp"
#include "level_runner.hpp"
#include "load_level.hpp"
#include "message_dialog.hpp"
#include "player_info.hpp"
#include "preferences.hpp"
#include "raster.hpp"
#include "sound.hpp"
#include "stats.hpp"
#include "surface_cache.hpp"
#include "wml_node.hpp"
#include "wml_writer.hpp"
#include "wml_utils.hpp"

namespace {
int global_pause_time;

typedef boost::function<void(const level&, screen_position&, float)> TransitionFn;

void transition_scene(level& lvl, screen_position& screen_pos, bool transition_out, TransitionFn draw_fn) {
	if(lvl.player()) {
		lvl.player()->get_entity().set_invisible(true);
	}

	for(int n = 0; n <= 20; ++n) {
		lvl.process();

		draw_fn(lvl, screen_pos, transition_out ? (n/20.0) : (1 - n/20.0));

		SDL_GL_SwapBuffers();
		SDL_Delay(20);
	}
	
	if(lvl.player()) {
		lvl.player()->get_entity().set_invisible(false);
	}
}

void fade_scene(const level& lvl, screen_position& screen_pos, float fade) {
	draw_scene(lvl, screen_pos);

	const SDL_Rect r = {0, 0, graphics::screen_width(), graphics::screen_height()};
	const SDL_Color c = {0,0,0,0};
	graphics::draw_rect(r, c, fade*255);
}

void flip_scene(const level& lvl, screen_position& screen_pos, float amount) {
	screen_pos.flip_rotate = amount*1000;
	draw_scene(lvl, screen_pos);
}

void iris_scene(const level& lvl, screen_position& screen_pos, float amount) {
	if(lvl.player() == NULL) {
		return;
	}

	//Enable the stencil buffer.
	glEnable(GL_STENCIL_TEST);
	glClear(GL_STENCIL_BUFFER_BIT);

	//Set things up so we will draw to the stencil buffer, not to the screen.
	glStencilFunc(GL_NEVER, 1, 1);
	glStencilOp(GL_REPLACE, GL_KEEP, GL_KEEP);

	glPushMatrix();

	glTranslatef(-screen_pos.x/100, -screen_pos.y/100, 0);

	point pos = lvl.player()->get_entity().midpoint();

	//Draw a circle.
	const float radius = 500 - amount*500;
	glBegin(GL_TRIANGLE_FAN);
	glVertex3f(pos.x, pos.y, 0);
	for(double angle = 0; angle < 3.1459*2.0; angle += 0.01) {
		const double x = pos.x + radius*cos(angle);
		const double y = pos.y + radius*sin(angle);
		glVertex3f(x, y, 0);
	}

	glEnd();

	glPopMatrix();

	//Now we've set the stencil to a circle, set things up so that the stencil
	//will be used.
	glStencilFunc(GL_EQUAL, 1, 1);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	
	draw_scene(lvl, screen_pos);

	glDisable(GL_STENCIL_TEST);
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

level_runner::level_runner(boost::scoped_ptr<level>& lvl, std::string& level_cfg)
  : lvl_(lvl), level_cfg_(level_cfg)
{
	quit_ = false;

	current_second_ = time(NULL);
	current_fps_ = 0;
	next_fps_ = 0;
	current_delay_ = 0;
	next_delay_ = 0;
	current_draw_ = 0;
	next_draw_ = 0;
	current_process_ = 0;
	next_process_ = 0;

	cycle = 0;
	paused = false;
	done = false;
	start_time_ = SDL_GetTicks();
	pause_time_ = -global_pause_time;
}

bool level_runner::play_level()
{
	while(!done && !quit_) {
		bool res = play_cycle();
		if(!res) {
			return quit_;
		}
	}

	return quit_;
}

bool level_runner::play_cycle()
{
	if(controls::first_invalid_cycle() >= 0) {
		lvl_->replay_from_cycle(controls::first_invalid_cycle());
		controls::mark_valid();
	}

	if(controls::num_players() > 1) {
		lvl_->backup();
	}

	const bool is_multiplayer = controls::num_players() > 1;

	int desired_end_time = start_time_ + pause_time_ + global_pause_time + cycle*20 + 20;

	if(!is_multiplayer) {
		const int ticks = SDL_GetTicks();
		if(desired_end_time < ticks) {
			const int new_desired_end_time = ticks + 20;
			pause_time_ += new_desired_end_time - desired_end_time;
			desired_end_time = new_desired_end_time;
		}
	}

	if(lvl_->players().size() == 1 && lvl_->player() && lvl_->player()->get_entity().hitpoints() <= 0) {
		//record stats of the player's death
		lvl_->player()->get_entity().record_stats_movement();
		stats::record_event(lvl_->id(), stats::record_ptr(new stats::die_record(lvl_->player()->get_entity().midpoint())));

		entity_ptr save = lvl_->player()->get_entity().save_condition();
		if(!save) {
			return false;
		}
		preload_level(save->get_player_info()->current_level());
		transition_scene(*lvl_, last_draw_position(), false, fade_scene);
		level* new_level = load_level(save->get_player_info()->current_level());
		sound::play_music(new_level->music());
		set_scene_title(new_level->title());
		new_level->add_player(save);
		save->save_game();
		lvl_.reset(new_level);
		last_draw_position() = screen_position();
	} else if(lvl_->players().size() > 1) {
		foreach(const entity_ptr& c, lvl_->players()) {
			if(c->hitpoints() <= 0) {
				//in multiplayer we respawn on death
				c->respawn_player();
			}
		}
	}

	const level::portal* portal = lvl_->get_portal();
	if(portal) {
		//we might want to change the portal, so copy it and make it mutable.
		level::portal mutable_portal = *portal;
		portal = &mutable_portal;

		level_cfg_ = portal->level_dest;
		if(level_cfg_.empty()) {
			//the portal is within the same level

			if(portal->dest_label.empty() == false) {
				const_entity_ptr dest_door = lvl_->get_entity_by_label(portal->dest_label);
				if(dest_door) {
					mutable_portal.dest = point(dest_door->x() + dest_door->teleport_offset_x()*dest_door->face_dir(), dest_door->y() + dest_door->teleport_offset_y());
					mutable_portal.dest_starting_pos = false;
				}
			}

			last_draw_position() = screen_position();
			player_info* player = lvl_->player();
			if(player) {
				player->get_entity().set_pos(portal->dest);
			}
		} else {
			//the portal is to another level
			preload_level(level_cfg_);

			const std::string transition = portal->transition;
			if(transition.empty() || transition == "fade") {
				transition_scene(*lvl_, last_draw_position(), true, fade_scene);
			} else if(transition == "flip") {
				transition_scene(*lvl_, last_draw_position(), true, flip_scene);
			} else if(transition == "iris") {
				transition_scene(*lvl_, last_draw_position(), true, iris_scene);
			} else if(transition == "instant") {
				//do nothing.
			}

			level* new_level = load_level(level_cfg_);

			if(portal->dest_label.empty() == false) {
				//the label of an object was specified as an entry point,
				//so set our position there.
				const_entity_ptr dest_door = new_level->get_entity_by_label(portal->dest_label);
				if(dest_door) {
					mutable_portal.dest = point(dest_door->x() + dest_door->teleport_offset_x()*dest_door->face_dir(), dest_door->y() + dest_door->teleport_offset_y());
					mutable_portal.dest_starting_pos = false;
				}
			}

			sound::play_music(new_level->music());
			set_scene_title(new_level->title());
			point dest = portal->dest;
			if(portal->dest_str.empty() == false) {
				dest = new_level->get_dest_from_str(portal->dest_str);
			} else if(portal->dest_starting_pos) {
				const player_info* new_player = new_level->player();
				if(new_player) {
					dest = point(new_player->get_entity().x(), new_player->get_entity().y());
				}
			}

			player_info* player = lvl_->player();
			if(player && portal->saved_game == false) {
				player->get_entity().set_pos(dest);
				new_level->add_player(&player->get_entity());
				player->get_entity().move_to_standing(*new_level);
				player->get_entity().handle_event("enter_level");
			} else {
				player = new_level->player();
			}

			//if we're in a multiplayer level then going through a portal
			//will take us out of multiplayer.
			if(lvl_->players().size() != new_level->players().size()) {
				lvl_.reset(new_level);
				done = true;
				throw multiplayer_exception();
			}

			lvl_.reset(new_level);
			last_draw_position() = screen_position();

			if(transition == "flip") {
				transition_scene(*lvl_, last_draw_position(), false, flip_scene);
			}

			if(done) {
				return false;
			}
		}
	}

	joystick::update();

	if(message_dialog::get() == NULL) {
		SDL_Event event;
		while(SDL_PollEvent(&event)) {
			switch(event.type) {
			case SDL_QUIT:
				done = true;
				quit_ = true;
				break;
			case SDL_VIDEORESIZE: {
				continue; //disabled.
				/*
				const SDL_ResizeEvent* const resize = reinterpret_cast<SDL_ResizeEvent*>(&event);
				screen_width = resize->w;
				screen_height = resize->h;
				if(screen_width > screen_height + screen_height/3) {
					screen_width = screen_height + screen_height/3;
				}

				if(screen_height > (screen_width*3)/4) {
					screen_height = (screen_width*3)/4;
				}
				SDL_SetVideoMode(screen_width,screen_height,0,SDL_OPENGL|(preferences::fullscreen() ? SDL_FULLSCREEN : 0));
				*/

			}
			case SDL_KEYDOWN: {
				const SDLMod mod = SDL_GetModState();
				const SDLKey key = event.key.keysym.sym;
				if(key == SDLK_ESCAPE) {
					//record a quit event in stats
					if(lvl_->player()) {
						lvl_->player()->get_entity().record_stats_movement();
						stats::record_event(lvl_->id(), stats::record_ptr(new stats::quit_record(lvl_->player()->get_entity().midpoint())));
					}

					done = true;
					quit_ = true;
					break;
				} else if(key == SDLK_e && (mod&KMOD_CTRL)) {
					pause_time_ -= SDL_GetTicks();
					editor::edit(lvl_->id().c_str(), last_draw_position().x/100, last_draw_position().y/100);
					lvl_.reset(load_level(editor::last_edited_level().c_str()));
					pause_time_ += SDL_GetTicks();
				} else if(key == SDLK_s && (mod&KMOD_CTRL)) {
					std::string data;
					
					wml::node_ptr lvl_node = wml::deep_copy(lvl_->write());
					wml::write(lvl_node, data);
					sys::write_file("save.cfg", data);
				} else if(key == SDLK_w && (mod&KMOD_CTRL)) {
					//warp to another level.
					std::vector<std::string> levels = get_known_levels();
					assert(!levels.empty());
					int index = std::find(levels.begin(), levels.end(), lvl_->id()) - levels.begin();
					index = (index+1)%levels.size();
					level* new_level = load_level(levels[index]);
					sound::play_music(new_level->music());
					set_scene_title(new_level->title());
					lvl_.reset(new_level);
				} else if(key == SDLK_l && (mod&KMOD_CTRL)) {
					preferences::set_use_pretty_scaling(!preferences::use_pretty_scaling());
					graphics::surface_cache::clear();
					graphics::texture::clear_cache();
				} else if(key == SDLK_i && lvl_->player()) {
					pause_scope pauser;
					show_inventory(*lvl_, lvl_->player()->get_entity());
				} else if(key == SDLK_m && mod & KMOD_CTRL) {
					sound::mute(!sound::muted()); //toggle sound
				} else if(key == SDLK_p && mod & KMOD_CTRL) {
					paused = !paused;
				} else if(key == SDLK_p && mod & KMOD_ALT) {
					preferences::set_use_pretty_scaling(!preferences::use_pretty_scaling());
					graphics::texture::clear_textures();
				} else if(key == SDLK_f && mod & KMOD_CTRL) {
					preferences::set_fullscreen(!preferences::fullscreen());
					SDL_SetVideoMode(graphics::screen_width(),graphics::screen_height(),0,SDL_OPENGL|(preferences::fullscreen() ? SDL_FULLSCREEN : 0));
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
		pause_time_ += 20;
	} else {
		if (!paused) {
			const int start_process = SDL_GetTicks();
			lvl_->process();
			next_process_ += (SDL_GetTicks() - start_process);
		} else {
			pause_time_ += 20;
		}
	}

	if(lvl_->end_game()) {
		transition_scene(*lvl_, last_draw_position(), false, fade_scene);
		show_end_game();
		done = true;
		return true;
	}

	const int start_draw = SDL_GetTicks();
	draw_scene(*lvl_, last_draw_position());
	next_draw_ += (SDL_GetTicks() - start_draw);

	performance_data perf = { current_fps_, current_delay_, current_draw_, current_process_, cycle };
	draw_fps(*lvl_, perf);
	
	SDL_GL_SwapBuffers();
	++next_fps_;

	const time_t this_second = time(NULL);
	if(this_second != current_second_) {
		current_second_ = this_second;
		current_fps_ = next_fps_;
		current_delay_ = next_delay_;
		current_draw_ = next_draw_;
		current_process_ = next_process_;
		next_fps_ = 0;
		next_delay_ = 0;
		next_draw_ = 0;
		next_process_ = 0;
	}

	const int raw_wait_time = desired_end_time - SDL_GetTicks();
	const int wait_time = std::max<int>(1, desired_end_time - SDL_GetTicks());
	next_delay_ += wait_time;
	SDL_Delay(wait_time);

	if (!paused) ++cycle;

	return !quit_;
}

namespace {
//variable to mark if we are already pausing. If so, don't pause again
//based on a new pause scope.
bool pause_scope_active = false;
}

pause_scope::pause_scope() : ticks_(SDL_GetTicks()), active_(!pause_scope_active)
{
	pause_scope_active = true;
}

pause_scope::~pause_scope()
{
	if(active_) {
		const int t = SDL_GetTicks() - ticks_;
		global_pause_time += t;
		pause_scope_active = false;
	}
}
