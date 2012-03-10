#include <math.h>

#include <boost/bind.hpp>
#include <boost/function.hpp>

#include "graphics.hpp"

#include "collision_utils.hpp"
#include "controls.hpp"
#include "custom_object.hpp"
#include "custom_object_functions.hpp"
#include "debug_console.hpp"
#include "draw_scene.hpp"
#ifndef NO_EDITOR
#include "editor.hpp"
#endif
#include "filesystem.hpp"
#include "font.hpp"
#include "foreach.hpp"
#include "formula_profiler.hpp"
#include "inventory.hpp"
#if defined(TARGET_OS_HARMATTAN) || defined(TARGET_BLACKBERRY)
#include "iphone_controls.hpp"
#endif
#ifdef TARGET_BLACKBERRY
#include "userevents.h"
#endif
#include "joystick.hpp"
#include "level_runner.hpp"
#include "light.hpp"
#include "load_level.hpp"
#include "message_dialog.hpp"
#include "object_events.hpp"
#include "pause_game_dialog.hpp"
#include "player_info.hpp"
#include "preferences.hpp"
#include "raster.hpp"
#include "settings_dialog.hpp"
#include "sound.hpp"
#include "stats.hpp"
#include "surface_cache.hpp"
#include "text_entry_widget.hpp"
#include "utils.hpp"
#include "wml_node.hpp"
#include "wml_writer.hpp"
#include "wml_utils.hpp"
#include "IMG_savepng.h"
#include "globals.h"

namespace {
int skipping_game = 0;

int global_pause_time;

typedef boost::function<void(const level&, screen_position&, float)> TransitionFn;

//prepare to call transition_scene by making sure that frame buffers are
//filled with the image of the screen.
void prepare_transition_scene(level& lvl, screen_position& screen_pos)
{
	draw_scene(lvl, screen_pos);
	SDL_GL_SwapBuffers();
	draw_scene(lvl, screen_pos);
	SDL_GL_SwapBuffers();
}

void transition_scene(level& lvl, screen_position& screen_pos, bool transition_out, TransitionFn draw_fn) {
	if(lvl.player()) {
		lvl.player()->get_entity().set_invisible(true);
	}

	const int start_time = SDL_GetTicks();

	for(int n = 0; n <= 20; ++n) {
//		lvl.process();

		draw_fn(lvl, screen_pos, transition_out ? (n/20.0) : (1 - n/20.0));

		SDL_GL_SwapBuffers();

		const int target_end_time = start_time + (n+1)*preferences::frame_time_millis();
		const int current_time = SDL_GetTicks();
		const int skip_time = target_end_time - current_time;
		if(skip_time > 0) {
			SDL_Delay(skip_time);
		}
	}
	
	if(lvl.player()) {
		lvl.player()->get_entity().set_invisible(false);
	}
}

void fade_scene(const level& lvl, screen_position& screen_pos, float fade) {
	const SDL_Rect r = {0, 0, graphics::screen_width(), graphics::screen_height()};
	const SDL_Color c = {0,0,0,0};
	graphics::draw_rect(r, c, 128*fade);
}

void flip_scene(const level& lvl, screen_position& screen_pos, float amount) {
	screen_pos.flip_rotate = amount*1000;
	draw_scene(lvl, screen_pos);
}

bool calculate_stencil_buffer_available() {
	GLint stencil_buffer_bits = 0;
	glGetIntegerv(GL_STENCIL_BITS, &stencil_buffer_bits);
	std::cerr << "stencil buffer size: " << stencil_buffer_bits << "\n";
	return stencil_buffer_bits > 0;	
}

void iris_scene(const level& lvl, screen_position& screen_pos, float amount) {
	if(lvl.player() == NULL) {
		return;
	}

	const_entity_ptr player = &lvl.player()->get_entity();
	const point light_pos = player->midpoint();

	if(amount >= 0.99) {
		SDL_Rect rect = {0, 0, graphics::screen_width(), graphics::screen_height()};
		graphics::draw_rect(rect, graphics::color_black());
	} else {
		draw_scene(lvl, screen_pos);

		const int screen_x = screen_pos.x/100;
		const int screen_y = screen_pos.y/100;

		float radius_scale = 1.0 - amount;
		const int radius = radius_scale*radius_scale*500;
		const int center_x = -screen_x + light_pos.x;
		const int center_y = -screen_y + light_pos.y;
		SDL_Rect center_rect = {center_x - radius, center_y - radius, radius*2, radius*2 };

		if(center_rect.y > 0) {
			SDL_Rect top_rect = {0, 0, graphics::screen_width(), center_rect.y};
			graphics::draw_rect(top_rect, graphics::color_black());
		}

		const int bot_rect_height = graphics::screen_height() - (center_rect.y + center_rect.h);
		if(bot_rect_height > 0) {
			SDL_Rect bot_rect = {0, graphics::screen_height() - bot_rect_height, graphics::screen_width(), bot_rect_height};
			graphics::draw_rect(bot_rect, graphics::color_black());
		}

		if(center_rect.x > 0) {
			SDL_Rect left_rect = {0, 0, center_rect.x, graphics::screen_height()};
			graphics::draw_rect(left_rect, graphics::color_black());
		}

		const int right_rect_width = graphics::screen_width() - (center_rect.x + center_rect.w);
		if(right_rect_width > 0) {
			SDL_Rect right_rect = {graphics::screen_width() - right_rect_width, 0, right_rect_width, graphics::screen_height()};
			graphics::draw_rect(right_rect, graphics::color_black());
		}

		static std::vector<float> x_angles;
		static std::vector<float> y_angles;

		if(x_angles.empty()) {
			for(float angle = 0; angle < 3.1459*2.0; angle += 0.2) {
				x_angles.push_back(cos(angle));
				y_angles.push_back(sin(angle));
			}
		}


		std::vector<GLfloat> varray;
		for(int n = 0; n != x_angles.size(); ++n) {
			const float xpos1 = center_x + radius*x_angles[n];
			const float ypos1 = center_y + radius*y_angles[n];
			const float xpos2 = center_x + (center_rect.w + radius)*x_angles[n];
			const float ypos2 = center_y + (center_rect.h + radius)*y_angles[n];
			varray.push_back(xpos1);
			varray.push_back(ypos1);
			varray.push_back(xpos2);
			varray.push_back(ypos2);
		}

		glColor4ub(0, 0, 0, 255);

		glDisable(GL_TEXTURE_2D);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);

		glVertexPointer(2, GL_FLOAT, 0, &varray.front());
		glDrawArrays(GL_TRIANGLE_STRIP, 0, varray.size()/2);

		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glEnable(GL_TEXTURE_2D);

		glColor4ub(255, 255, 255, 255);
	}
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

void begin_skipping_game() {
	++skipping_game;
}

void end_skipping_game() {
	skipping_game = 0;
}

bool is_skipping_game() {
	return skipping_game > 0;
}

level_runner::level_runner(boost::intrusive_ptr<level>& lvl, std::string& level_cfg, std::string& original_level_cfg)
  : lvl_(lvl), level_cfg_(level_cfg), original_level_cfg_(original_level_cfg)
{
	quit_ = false;

	current_second_ = time(NULL);
	current_fps_ = 0;
	next_fps_ = 0;
	current_cycles_ = 0;
	next_cycles_ = 0;
	current_delay_ = 0;
	next_delay_ = 0;
	current_draw_ = 0;
	next_draw_ = 0;
	current_flip_ = 0;
	next_flip_ = 0;
	current_process_ = 0;
	next_process_ = 0;
	current_events_ = 0;

	nskip_draw_ = 0;

	cycle = 0;
	die_at = -1;
	paused = false;
	done = false;
	start_time_ = SDL_GetTicks();
	pause_time_ = -global_pause_time;
}

bool level_runner::play_level()
{
	sound::stop_looped_sounds(NULL);

	lvl_->set_as_current_level();
	while(!done && !quit_) {
		bool res = play_cycle();
		if(!res) {
			return quit_;
		}
	}

	return quit_;
}

namespace {
void load_level_thread(const std::string& lvl, level** res) {
	try {
		*res = load_level(lvl);
	} catch(const graphics::texture::worker_thread_error&) {
		std::cerr << "LOAD LEVEL FAILED: MUST DO IN MAIN THREAD\n";
	}
}
}

bool level_runner::play_cycle()
{
	static settings_dialog settings_dialog;
	if(controls::first_invalid_cycle() >= 0) {
		lvl_->replay_from_cycle(controls::first_invalid_cycle());
		controls::mark_valid();
	}

	if(controls::num_players() > 1) {
		lvl_->backup();
	}

	const bool is_multiplayer = controls::num_players() > 1;

	int desired_end_time = start_time_ + pause_time_ + global_pause_time + cycle*preferences::frame_time_millis() + preferences::frame_time_millis();

	if(!is_multiplayer) {
		const int ticks = SDL_GetTicks();
		if(desired_end_time < ticks) {
			const int new_desired_end_time = ticks + preferences::frame_time_millis();
			pause_time_ += new_desired_end_time - desired_end_time;
			desired_end_time = new_desired_end_time;
		}
	}

	//record player movement every 10 cycles on average.
	//TODO: currently disabled, since this takes up way too much space.
	//later work out a nicer way to do move events.
#if !TARGET_OS_HARMATTAN && !TARGET_OS_IPHONE
	if(rand()%10 == 0 && lvl_->player()) {
		point p = lvl_->player()->get_entity().midpoint();

		if(last_stats_point_level_ == lvl_->id()) {
			stats::entry("move").add_player_pos();
		}

		last_stats_point_ = p;
		last_stats_point_level_ = lvl_->id();
	}
#endif

	if(die_at <= 0 && lvl_->players().size() == 1 && lvl_->player() && lvl_->player()->get_entity().hitpoints() <= 0) {
		die_at = cycle;
	}

	if(die_at > 0 && cycle >= die_at + 30) {
		die_at = -1;

		foreach(entity_ptr e, lvl_->get_chars()) {
			e->handle_event(OBJECT_EVENT_PLAYER_DEATH);
		}

		//record stats of the player's death
		lvl_->player()->get_entity().record_stats_movement();
		stats::entry("die").add_player_pos();
		last_stats_point_level_ = "";

		entity_ptr save = lvl_->player()->get_entity().save_condition();
		if(!save) {
			return false;
		}

		prepare_transition_scene(*lvl_, last_draw_position());

		preload_level(save->get_player_info()->current_level());
		transition_scene(*lvl_, last_draw_position(), true, fade_scene);
		sound::stop_looped_sounds(NULL);
		level* new_level = load_level(save->get_player_info()->current_level());

		if(!new_level->music().empty()) {
			sound::play_music(new_level->music());
		}

		set_scene_title(new_level->title());
		new_level->add_player(save);
		new_level->set_as_current_level();
		save->save_game();
		save->handle_event(OBJECT_EVENT_LOAD_CHECKPOINT);
		place_entity_in_level(*new_level, *save);
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
				if(!player->get_entity().no_move_to_standing()){
					player->get_entity().move_to_standing(*lvl_);
				}
			}
		} else {
			//the portal is to another level
			
			if (preferences::load_compiled())
			{
				level::summary summary = level::get_summary(level_cfg_);
				if(!summary.music.empty()) {
					sound::play_music(summary.music);
				}
			}

			prepare_transition_scene(*lvl_, last_draw_position());

			const std::string transition = portal->transition;
			if(transition == "flip") {
				transition_scene(*lvl_, last_draw_position(), true, flip_scene);
			} else if(transition == "instant") {
				//do nothing
			} else if(transition != "fade") {
				transition_scene(*lvl_, last_draw_position(), true, iris_scene);
			} else {
				preload_level(level_cfg_);
				transition_scene(*lvl_, last_draw_position(), true, fade_scene);
			}

			sound::stop_looped_sounds(NULL);

			boost::intrusive_ptr<level> new_level(load_level(level_cfg_));
			if (!preferences::load_compiled() && !new_level->music().empty())
				sound::play_music(new_level->music());

			if(portal->dest_label.empty() == false) {
				//the label of an object was specified as an entry point,
				//so set our position there.
				const_entity_ptr dest_door = new_level->get_entity_by_label(portal->dest_label);
				if(dest_door) {
					mutable_portal.dest = point(dest_door->x() + dest_door->teleport_offset_x()*dest_door->face_dir(), dest_door->y() + dest_door->teleport_offset_y());
					mutable_portal.dest_starting_pos = false;
				}
			}

			new_level->set_as_current_level();

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
				if(!player->get_entity().no_move_to_standing()){
					player->get_entity().move_to_standing(*new_level);
				}
				player->get_entity().handle_event("enter_level");
			} else {
				player = new_level->player();
			}

			//if we're in a multiplayer level then going through a portal
			//will take us out of multiplayer.
			if(lvl_->players().size() != new_level->players().size()) {
				lvl_ = new_level;
				done = true;
				throw multiplayer_exception();
			}

			lvl_ = new_level;
			last_draw_position() = screen_position();

			if(transition == "flip") {
				transition_scene(*lvl_, last_draw_position(), false, flip_scene);
			}

			//we always want to exit this function so that we don't
			//draw the new level when it hasn't had a chance to process.
			return !done;
		}
	}

	joystick::update();
	bool should_pause = false;

	if(message_dialog::get() == NULL) {
		SDL_Event event;
		while(SDL_PollEvent(&event)) {
		//std::cerr << "Got event (level_runner 502): " << (int) event.type << ".\n";
#if TARGET_IPHONE_SIMULATOR || TARGET_OS_HARMATTAN || TARGET_OS_IPHONE
			should_pause = settings_dialog.handle_event(event);
#endif
			switch(event.type) {
			case SDL_QUIT: {
				stats::entry("quit").add_player_pos();
				done = true;
				quit_ = true;
				break;
			}
			case SDL_VIDEORESIZE: {
				const SDL_ResizeEvent* const resize = reinterpret_cast<SDL_ResizeEvent*>(&event);
				int width = resize->w;
				int height = resize->h;

				const int aspect = (preferences::actual_screen_width()*1000)/preferences::actual_screen_height();

				if(preferences::actual_screen_width()*preferences::actual_screen_height() < width*height) {
					//making the window larger
					if((height*aspect)/1000 > width) {
						width = (height*aspect)/1000;
					} else if((height*aspect)/1000 < width) {
						height = (width*1000)/aspect;
					}
				} else {
					//making the window smaller
					if((height*aspect)/1000 > width) {
						height = (width*1000)/aspect;
					} else if((height*aspect)/1000 < width) {
						width = (height*aspect)/1000;
					}
				}

				//make sure we don't have some ugly fractional aspect ratio
				while((width*1000)/height != aspect) {
					++width;
					height = (width*1000)/aspect;
				}

				preferences::set_actual_screen_width(width);
				preferences::set_actual_screen_height(height);

				graphics::set_video_mode(width, height);
				continue;
			}
#if TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE
			// make sure nothing happens while the app is supposed to be "inactive"
			case SDL_WINDOWEVENT:
				if (event.window.event == SDL_WINDOWEVENT_MINIMIZED)
				{
					SDL_Event e;
					while (SDL_WaitEvent(&e))
					{
						if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_RESTORED)
							break;
					}
				}
			break;
#elif TARGET_OS_HARMATTAN
			// make sure nothing happens while the app is supposed to be "inactive"
			case SDL_ACTIVEEVENT:
				if (event.active.state & SDL_APPINPUTFOCUS && event.active.gain == 0)
				{
					SDL_Event e;
					while (SDL_WaitEvent(&e))
					{
						if (e.type == SDL_ACTIVEEVENT && e.active.state & SDL_APPINPUTFOCUS && e.active.gain == 1)
							break;
					}
				}
			break;
#elif TARGET_BLACKBERRY
			// make sure nothing happens while the app is supposed to be "inactive"
			case SDL_ACTIVEEVENT:
				if (event.active.state & SDL_APPINPUTFOCUS && event.active.gain == 0)
				{
					write_autosave();
					preferences::save_preferences();

					SDL_Event e;
					while (SDL_WaitEvent(&e))
					{
						if (e.type == SDL_ACTIVEEVENT && e.active.state & SDL_APPINPUTFOCUS && e.active.gain == 1)
							break;
					}
				}
			break;
			case SDL_USEREVENT:
				if(event.user.code == ST_EVENT_SWIPE_DOWN) {
					should_pause = true;
				}
			break;
#endif
			case SDL_KEYDOWN: {
				const SDLMod mod = SDL_GetModState();
				const SDLKey key = event.key.keysym.sym;
				//std::cerr << "Key #" << (int) key << ".\n";
				if(key == SDLK_ESCAPE) {
					should_pause = true;
					break;
				} else if(key == SDLK_d && (mod&KMOD_CTRL)) {
					show_debug_console();

				} else if(key == SDLK_e && (mod&KMOD_CTRL)) {
					#ifndef NO_EDITOR
					pause_time_ -= SDL_GetTicks();
					editor::edit(lvl_->id().c_str(), last_draw_position().x/100, last_draw_position().y/100);
					lvl_.reset(load_level(editor::last_edited_level().c_str()));
					lvl_->set_as_current_level();
					if(lvl_->player()) {
						//we want to save the game after leaving the editor
						//so the game is restored to here if we die, rather
						//than going to some other level. We must run
						//the level through a process cycle just to make
						//sure everything is set properly for the player.
						lvl_->process();
						lvl_->player()->get_entity().save_game();
					}
					pause_time_ += SDL_GetTicks();
					#endif
				} else if(key == SDLK_s && (mod&KMOD_CTRL)) {
					std::cerr << "SAVING...\n";
					std::string data;
					
					wml::node_ptr lvl_node = wml::deep_copy(lvl_->write());
					if(sound::current_music().empty() == false) {
						lvl_node->set_attr("music", sound::current_music());
					}
					wml::write(lvl_node, data);
					sys::write_file(preferences::save_file_path(), data);
				} else if(key == SDLK_s && (mod&KMOD_ALT)) {
					IMG_SaveFrameBuffer((std::string(preferences::user_data_path()) + "screenshot.png").c_str(), 5);
				} else if(key == SDLK_w && (mod&KMOD_CTRL)) {
					//warp to another level.
					std::vector<std::string> levels = get_known_levels();
					assert(!levels.empty());
					int index = std::find(levels.begin(), levels.end(), lvl_->id()) - levels.begin();
					index = (index+1)%levels.size();
					level* new_level = load_level(levels[index]);
					new_level->set_as_current_level();

					if(!new_level->music().empty()) {
						sound::play_music(new_level->music());
					}

					set_scene_title(new_level->title());
					lvl_.reset(new_level);
				} else if(key == SDLK_l && (mod&KMOD_CTRL)) {
					preferences::set_use_pretty_scaling(!preferences::use_pretty_scaling());
					graphics::surface_cache::clear();
					graphics::texture::clear_cache();
				} else if(key == SDLK_i && lvl_->player()) {
// INVENTORY CURRENTLY DISABLED
//					pause_scope pauser;
//					show_inventory(*lvl_, lvl_->player()->get_entity());
				} else if(key == SDLK_m && mod & KMOD_CTRL) {
					sound::mute(!sound::muted()); //toggle sound
				} else if(key == SDLK_p && mod & KMOD_CTRL) {
					paused = !paused;
				} else if(key == SDLK_p && mod & KMOD_ALT) {
					preferences::set_use_pretty_scaling(!preferences::use_pretty_scaling());
					graphics::texture::clear_textures();
				} else if(key == SDLK_f && mod & KMOD_CTRL) {
					preferences::set_fullscreen(!preferences::fullscreen());
					graphics::set_video_mode(graphics::screen_width(), graphics::screen_height());
				}
				break;
			}
#if defined(TARGET_OS_HARMATTAN) || defined(TARGET_BLACKBERRY)
			case SDL_MOUSEMOTION:
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
				iphone_controls::handle_event(event);
				break;
#endif
			default:
				break;
			}
		}
		
		if (should_pause)
		{
			settings_dialog.reset();
			const PAUSE_GAME_RESULT result = show_pause_game_dialog();

			handle_pause_game_result(result);
		}
	}

	if(message_dialog::get()) {
		message_dialog::get()->process();
		pause_time_ += preferences::frame_time_millis();
	} else {
		if (!paused && pause_stack == 0) {
			const int start_process = SDL_GetTicks();

			try {
				lvl_->process();
			} catch(interrupt_game_exception& e) {
				handle_pause_game_result(e.result);
			}

			next_process_ += (SDL_GetTicks() - start_process);
		} else {
			pause_time_ += preferences::frame_time_millis();
		}
	}

	if(lvl_->end_game()) {
		transition_scene(*lvl_, last_draw_position(), false, fade_scene);
		show_end_game();
		done = true;
		return true;
	}

	const int MaxSkips = 3;

	const int start_draw = SDL_GetTicks();
	if(start_draw < desired_end_time || nskip_draw_ >= MaxSkips) {
		const bool should_draw = update_camera_position(*lvl_, last_draw_position(), NULL, !is_skipping_game());
		lvl_->process_draw();

		if(should_draw) {
			render_scene(*lvl_, last_draw_position(), NULL, !is_skipping_game());
		}

		performance_data perf = { current_fps_, current_cycles_, current_delay_, current_draw_, current_process_, current_flip_, cycle, current_events_, profiling_summary_ };
#if TARGET_IPHONE_SIMULATOR || TARGET_OS_HARMATTAN || TARGET_OS_IPHONE
		if( ! is_achievement_displayed() ){
			settings_dialog.draw(in_speech_dialog());
		}
#endif
		
		if(!is_skipping_game() && preferences::show_fps()) {
			draw_fps(*lvl_, perf);
		}

		next_draw_ += (SDL_GetTicks() - start_draw);

		const int start_flip = SDL_GetTicks();
		if(!is_skipping_game()) {
			SDL_GL_SwapBuffers();
		}

		next_flip_ += (SDL_GetTicks() - start_flip);
		++next_fps_;
		nskip_draw_ = 0;
	} else {
		++nskip_draw_;
	}

	++next_cycles_;

	const time_t this_second = time(NULL);
	if(this_second != current_second_) {
		current_second_ = this_second;
		current_fps_ = next_fps_;
		current_cycles_ = next_cycles_;
		current_delay_ = next_delay_;
		current_draw_ = next_draw_;
		current_flip_ = next_flip_;
		current_process_ = next_process_;
		current_events_ = custom_object::events_handled_per_second;
		next_fps_ = 0;
		next_cycles_ = 0;
		next_delay_ = 0;
		next_draw_ = 0;
		next_process_ = 0;
		next_flip_ = 0;
		custom_object::events_handled_per_second = 0;

		profiling_summary_ = formula_profiler::get_profile_summary();
	}

	const int raw_wait_time = desired_end_time - SDL_GetTicks();
	const int wait_time = std::max<int>(1, desired_end_time - SDL_GetTicks());
	next_delay_ += wait_time;
	if (wait_time != 1 && !is_skipping_game()) {
		SDL_Delay(wait_time);
	}
	
	if(is_skipping_game()) {
		const int adjust_time = desired_end_time - SDL_GetTicks();
		if(adjust_time > 0) {
			pause_time_ -= adjust_time;
		}
	}

	if (!paused && pause_stack == 0) ++cycle;

	
#if TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE
	if (quit_)
	{
		write_autosave();
		preferences::save_preferences();
	}
#endif
	
	return !quit_;
}

void level_runner::show_debug_console()
{
#ifndef NO_EDITOR
	pause_time_ -= SDL_GetTicks();

	if(lvl_->player()) {
		debug_console::show_interactive_console(*lvl_, lvl_->player()->get_entity());
	}

	pause_time_ += SDL_GetTicks();
#endif
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

void level_runner::handle_pause_game_result(PAUSE_GAME_RESULT result)
{
	if(result == PAUSE_GAME_QUIT) {
		//record a quit event in stats
		if(lvl_->player()) {
			lvl_->player()->get_entity().record_stats_movement();
			stats::entry("quit").add_player_pos();
		}
		
		done = true;
		quit_ = true;
	} else if(result == PAUSE_GAME_GO_TO_TITLESCREEN) {
		done = true;
		original_level_cfg_ = "titlescreen.cfg";
	}
}
