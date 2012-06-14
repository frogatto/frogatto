#include <math.h>
#include <climits>

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
#include "formula_callable.hpp"
#if defined(TARGET_OS_HARMATTAN) || defined(TARGET_BLACKBERRY) || defined(__ANDROID__)
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
#include "utils.hpp"
#include "variant_utils.hpp"
#include "IMG_savepng.h"
#include "globals.h"
#include "texture.hpp"

namespace {
level_runner* current_level_runner = NULL;

class current_level_runner_scope {
	level_runner* old_;
public:
	current_level_runner_scope(level_runner* value) : old_(current_level_runner)
	{
		current_level_runner = value;
	}

	~current_level_runner_scope() {
		current_level_runner = old_;
	}
};

int skipping_game = 0;

int global_pause_time;

typedef boost::function<void(const level&, screen_position&, float)> TransitionFn;

//prepare to call transition_scene by making sure that frame buffers are
//filled with the image of the screen.
void prepare_transition_scene(level& lvl, screen_position& screen_pos)
{
	draw_scene(lvl, screen_pos);
	SDL_GL_SwapBuffers();
#if defined(__ANDROID__)
    graphics::reset_opengl_state();
#endif
	draw_scene(lvl, screen_pos);
	SDL_GL_SwapBuffers();
#if defined(__ANDROID__)
    graphics::reset_opengl_state();
#endif
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
#if defined(__ANDROID__)
		graphics::reset_opengl_state();
#endif

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
#if defined(__ANDROID__)
		graphics::reset_opengl_state();
#endif
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

void translate_mouse_event(SDL_Event *ev)
{
	if(ev->type == SDL_MOUSEMOTION) {
#if TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE
		ev->motion.x = (ev.motion.x*graphics::screen_width())/preferences::virtual_screen_width() + last_draw_position().x/100;
		ev->motion.y = (ev.motion.y*graphics::screen_height())/preferences::virtual_screen_height() + last_draw_position().y/100;
#else
		ev->motion.x = (ev->motion.x*preferences::virtual_screen_width())/preferences::actual_screen_width() + last_draw_position().x/100;
		ev->motion.y = (ev->motion.y*preferences::virtual_screen_height())/preferences::actual_screen_height() + last_draw_position().y/100;
#endif
	} else if(ev->type == SDL_MOUSEBUTTONDOWN || ev->type == SDL_MOUSEBUTTONUP) {
#if TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE
		ev.button.x = (ev.button.x*graphics::screen_width())/preferences::virtual_screen_width() + last_draw_position().x/100;
		ev.button.y = (ev.button.y*graphics::screen_height())/preferences::virtual_screen_height() + last_draw_position().y/100;
#else
		ev->button.x = (ev->button.x*preferences::virtual_screen_width())/preferences::actual_screen_width() + last_draw_position().x/100;
		ev->button.y = (ev->button.y*preferences::virtual_screen_height())/preferences::actual_screen_height() + last_draw_position().y/100;
#endif
	}
	//x = (x*graphics::screen_width())/preferences::virtual_screen_width() + last_draw_position().x/100;
	//y = (y*graphics::screen_height())/preferences::virtual_screen_height() + last_draw_position().y/100;
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

void video_resize( SDL_Event &event ) 
{
    const SDL_ResizeEvent* resize = reinterpret_cast<SDL_ResizeEvent*>(&event);
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
}

bool zorder_compare(entity_ptr e1, entity_ptr e2)
{
	// Compare z-orders, then sub-z-orders then vertical mid-points.
	if(e1->zorder() > e2->zorder()) {
		return true;
	} else if(e1->zorder() == e2->zorder()) {
		if(e1->zsub_order() > e2->zsub_order()) {
			return true;
		} else if(e1->zsub_order() == e2->zsub_order()) {
			if(e1->midpoint().y > e2->midpoint().y) {
				return true;
			}
		}
	}
	return false;
}

bool level_runner::handle_mouse_events(const SDL_Event &event)
{
	static const int MouseDownEventID = get_object_event_id("mouse_down");
	static const int MouseUpEventID = get_object_event_id("mouse_up");
	static const int MouseMoveEventID = get_object_event_id("mouse_move");
	static const int MouseDownEventAllID = get_object_event_id("mouse_down*");
	static const int MouseUpEventAllID = get_object_event_id("mouse_up*");
	static const int MouseMoveEventAllID = get_object_event_id("mouse_move*");


	static const int MouseEnterID = get_object_event_id("mouse_enter");
	static const int MouseLeaveID = get_object_event_id("mouse_leave");

	static const int MouseClickID = get_object_event_id("click");
	//static const int MouseDblClickID = get_object_event_id("dblclick");
	//static const int MouseDragID = get_object_event_id("drag");
	//static const int MouseDragID = get_object_event_id("drag_start");
	//static const int MouseDragID = get_object_event_id("drag_end");

	if(paused) {
		// skip mouse event handling when paused.
		// XXX: when we become unpaused we need to reset the state of drag operations
		// and partial clicks.
		return false;
	}

	switch(event.type)
	{
#if defined(__ANDROID__)
		case SDL_JOYBUTTONDOWN:
		case SDL_JOYBUTTONUP:
		case SDL_JOYBALLMOTION:
			break;
#else
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
		case SDL_MOUSEMOTION:
		    int x = event.type == SDL_MOUSEMOTION ? event.motion.x : event.button.x;
			int y = event.type == SDL_MOUSEMOTION ? event.motion.y : event.button.y;
			int i = event.type == SDL_MOUSEMOTION ? event.motion.which : event.button.which;
			const int basic_evt = event.type == SDL_MOUSEBUTTONDOWN 
				? MouseDownEventID 
				: event.type == SDL_MOUSEMOTION
					? MouseMoveEventID : MouseUpEventID;
			const int catch_all_event = event.type == SDL_MOUSEBUTTONDOWN 
				? MouseDownEventAllID 
				: event.type == SDL_MOUSEMOTION
					? MouseMoveEventAllID : MouseUpEventAllID;
			if(!lvl_->gui_event(event)) {
				x = (x*graphics::screen_width())/preferences::virtual_screen_width() + last_draw_position().x/100;
				y = (y*graphics::screen_height())/preferences::virtual_screen_height() + last_draw_position().y/100;
				game_logic::map_formula_callable* callable = new game_logic::map_formula_callable;
				callable->add("mouse_x", variant(x));
				callable->add("mouse_y", variant(y));
				callable->add("mouse_index", variant(i));
				if(event.type != SDL_MOUSEMOTION) {
					callable->add("mouse_button", variant(event.button.button));
				}
				variant v(callable);
				std::vector<variant> items;
				std::vector<entity_ptr> cs = lvl_->get_characters_at_point(x, y, last_draw_position().x/100, last_draw_position().y/100);
				std::sort(cs.begin(), cs.end(), zorder_compare);
				std::vector<entity_ptr>::iterator it;
				bool handled = false;
				bool click_handled = false;
				for(it = cs.begin(); it != cs.end(); ++it) {
					(*it)->handle_event(basic_evt, callable);
					// XXX: fix this for dragging(with multiple mice)
					if(event.type == SDL_MOUSEBUTTONUP && !click_handled /*&& !dragging*/) {
						(*it)->handle_event(MouseClickID, callable);
						if((*it)->mouse_event_swallowed()) {
							click_handled = true;
						}
					}
					handled = true;
					items.push_back(variant((*it).get()));
				}
				callable->add("handled", variant(handled));
				variant obj_ary(&items);
				callable->add("objects_under_mouse", obj_ary);
				foreach(entity_ptr object, level::current().get_chars()) {
					object->handle_event(catch_all_event, callable);
				}
			}
			break;
#endif
	}
	return false;
}

level_runner* level_runner::get_current()
{
	return current_level_runner;
}

level_runner::level_runner(boost::intrusive_ptr<level>& lvl, std::string& level_cfg, std::string& original_level_cfg)
  : lvl_(lvl), level_cfg_(level_cfg), original_level_cfg_(original_level_cfg),
    editor_(NULL)
#ifndef NO_EDITOR
	, history_trails_state_id_(-1), object_reloads_state_id_(-1),
	tile_rebuild_state_id_(-1)
#endif
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

void level_runner::start_editor()
{
#ifndef NO_EDITOR
	if(!editor_) {
		controls::control_backup_scope ctrl_backup;
		editor_ = editor::get_editor(lvl_->id().c_str());
		editor_resolution_manager_.reset(new editor_resolution_manager(editor_->xres(), editor_->yres()));
		editor_->set_playing_level(lvl_);
		editor_->setup_for_editing();
		lvl_->set_editor();
		lvl_->set_as_current_level();
		init_history_slider();
	} else {
		//Pause the game and set the level to its original
		//state if the user presses ctrl+e twice.
		paused = !paused;
		editor_->reset_playing_level(false);
		last_draw_position().init = false;
		init_history_slider();
		if(!paused) {
			controls::read_until(lvl_->cycle());
		}
	}
#endif
}

void level_runner::close_editor()
{
	editor_ = NULL;
	history_slider_.reset();
	history_button_.reset();
	history_trails_.clear();
	editor_resolution_manager_.reset();
	lvl_->mutate_value("zoom", variant(1));
	lvl_->set_editor(false);
	paused = false;
	controls::read_until(lvl_->cycle());
	init_history_slider();
}

bool level_runner::play_level()
{
	const current_level_runner_scope current_level_runner_setter(this);

	sound::stop_looped_sounds(NULL);

	lvl_->set_as_current_level();
	bool reversing = false;

	if(preferences::edit_on_start()) {
		start_editor();	
	}

#ifndef NO_EDITOR
	if(!lvl_->player()) {
		controls::control_backup_scope ctrl_backup;
		paused = true;
		editor_ = editor::get_editor(lvl_->id().c_str());
		editor_resolution_manager_.reset(new editor_resolution_manager(editor_->xres(), editor_->yres()));
		editor_->set_playing_level(lvl_);
		editor_->setup_for_editing();
		lvl_->set_editor();
	}
#endif

	while(!done && !quit_) {
		if(key[SDLK_t] && preferences::record_history()
#ifndef NO_EDITOR
			&& (!editor_ || !editor_->has_keyboard_focus())
			&& (!console_ || !console_->has_keyboard_focus())
#endif
		) {
				if(!reversing) {
					pause_time_ -= SDL_GetTicks();
				}
				reverse_cycle();
				reversing = true;
		} else {
			if(reversing) {
				controls::read_until(lvl_->cycle());
				pause_time_ += SDL_GetTicks();
			}
			reversing = false;
			bool res = play_cycle();
			if(!res) {
				return quit_;
			}

			if(preferences::record_history()) {
				lvl_->backup();
			}
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

	performance_data current_perf(current_fps_,50,0,0,0,0,0,custom_object::events_handled_per_second,"");

	if(controls::num_players() > 1) {
		lvl_->backup();
	}

	boost::scoped_ptr<controls::local_controls_lock> controls_lock;
#ifndef NO_EDITOR
	if(editor_ && editor_->has_keyboard_focus() ||
	   console_ && console_->has_keyboard_focus()) {
		controls_lock.reset(new controls::local_controls_lock);
	}

	if(editor_) {

		controls::control_backup_scope ctrl_backup;
		editor_->set_pos(last_draw_position().x/100 - (editor_->zoom()-1)*(graphics::screen_width()-editor::sidebar_width())/2, last_draw_position().y/100 - (editor_->zoom()-1)*(graphics::screen_height())/2);
		editor_->process();
		lvl_->complete_rebuild_tiles_in_background();
		lvl_->set_as_current_level();

		lvl_->mutate_value("zoom", variant(decimal(1.0/editor_->zoom())));

		custom_object_type::reload_modified_code();

		if(history_trails_.empty() == false && (tile_rebuild_state_id_ != level::tile_rebuild_state_id() || history_trails_state_id_ != editor_->level_state_id() || object_reloads_state_id_ != custom_object_type::num_object_reloads())) {
			update_history_trails();
		}
	}
#endif

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

	if(editor_ && die_at > 0 && cycle >= die_at + 30) {
#ifndef NO_EDITOR
		die_at = -1;

		//If the player dies in the editor, return this level to its
		//initial state.
		editor_->reset_playing_level(false);
		last_draw_position().init = false;
#endif

	} else if(die_at > 0 && cycle >= die_at + 30) {
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
#ifndef NO_EDITOR
			if(editor_) {
				editor_->confirm_quit(false);
			}
#endif
			
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

			if(editor_) {
				new_level->set_editor();
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
#ifndef NO_EDITOR
			if(editor_) {
				editor_ = editor::get_editor(lvl_->id().c_str());
				editor_->set_playing_level(lvl_);
				editor_->setup_for_editing();
				lvl_->set_as_current_level();
				lvl_->set_editor();
				init_history_slider();
			}
#endif

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
			//std::cerr << "Got event (level_runner 672): " << (int) event.type << ".\n";
#if TARGET_IPHONE_SIMULATOR || TARGET_OS_HARMATTAN || TARGET_OS_IPHONE
			should_pause = settings_dialog.handle_event(event);
#endif
#ifndef NO_EDITOR
			bool swallowed = false;
			if(console_) {
				swallowed = console_->process_event(event, swallowed);
			}

			if(history_slider_ && paused) {
				swallowed = history_slider_->process_event(event, swallowed) || swallowed;
				swallowed = history_button_->process_event(event, false) || swallowed;
			}

			if(editor_) {
				swallowed = editor_->handle_event(event, swallowed) || swallowed;
				lvl_->set_as_current_level();

				if(editor::last_edited_level() != lvl_->id() && editor_->confirm_quit()) {
					level* new_level = load_level(editor::last_edited_level());
					if(editor_) {
						new_level->set_editor();
					}

					new_level->set_as_current_level();

					if(!new_level->music().empty()) {
						sound::play_music(new_level->music());
					}

					set_scene_title(new_level->title());
					lvl_.reset(new_level);

					lvl_->editor_clear_selection();
					editor_ = editor::get_editor(lvl_->id().c_str());
					editor_->set_playing_level(lvl_);
					editor_->setup_for_editing();
					lvl_->set_as_current_level();
					lvl_->set_editor();
					init_history_slider();
				}

				if(editor_->done()) {
					close_editor();
				}
			}

#endif
			{
				// pre-translate the mouse positions.
				SDL_Event ev(event);
				translate_mouse_event(&ev);
				foreach(const entity_ptr& e, lvl_->get_active_chars()) {
					custom_object* custom_obj = dynamic_cast<custom_object*>(e.get());
					swallowed = custom_obj->handle_sdl_event(ev, swallowed);
				}
			}

			if(swallowed) {
				continue;
			}

			switch(event.type) {
			case SDL_QUIT: {
				stats::entry("quit").add_player_pos();
				done = true;
				quit_ = true;
				break;
			}
			case SDL_VIDEORESIZE: video_resize( event ); continue;
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
#elif defined(__ANDROID__)
			// make sure nothing happens while the app is supposed to be "inactive"
			case SDL_ACTIVEEVENT:
				if (event.active.state & SDL_APPACTIVE && !event.active.gain)
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
#endif
			case SDL_KEYDOWN: {
				const SDLMod mod = SDL_GetModState();
				const SDLKey key = event.key.keysym.sym;
				//std::cerr << "Key #" << (int) key << ".\n";
				if(key == SDLK_ESCAPE) {
					if(editor_) {
#ifndef NO_EDITOR
						close_editor();
#endif
					} else {
						should_pause = true;
					}
					break;
				} else if(key == SDLK_d && (mod&KMOD_CTRL)) {
#ifndef NO_EDITOR
					if(!console_ && lvl_->player()) {
						console_.reset(new debug_console::console_dialog(*lvl_, lvl_->player()->get_entity()));
					} else {
						console_.reset();
					}
#endif

				} else if(key == SDLK_e && (mod&KMOD_CTRL)) {
#ifndef NO_EDITOR
					start_editor();
#endif
				} else if(key == SDLK_r && (mod&KMOD_CTRL) && editor_) {
#ifndef NO_EDITOR
					//We're in the editor and we want to refresh the level
					//to its original state. If alt is held, we also
					//reset the player.
					const bool reset_pos = mod&KMOD_ALT;
					editor_->reset_playing_level(!reset_pos);

					if(reset_pos) {
						//make the camera jump to the player
						last_draw_position().init = false;
					}
#endif
				} else if(key == SDLK_s && (mod&KMOD_CTRL) && !editor_) {
					std::cerr << "SAVING...\n";
					std::string data;
					
					variant lvl_node = lvl_->write();
					if(sound::current_music().empty() == false) {
						lvl_node = lvl_node.add_attr(variant("music"), variant(sound::current_music()));
					}
					sys::write_file(preferences::save_file_path(), lvl_node.write_json(true));
				} else if(key == SDLK_s && (mod&KMOD_ALT)) {
					IMG_SaveFrameBuffer((std::string(preferences::user_data_path()) + "screenshot.png").c_str(), 5);
				} else if(key == SDLK_w && (mod&KMOD_CTRL)) {
#ifndef NO_EDITOR
					if(editor_) {
						if(!editor_->confirm_quit()) {
							break;
						}
					}
#endif

					//warp to another level.
					std::vector<std::string> levels = get_known_levels();
					assert(!levels.empty());
					int index = std::find(levels.begin(), levels.end(), lvl_->id()) - levels.begin();
					index = (index+1)%levels.size();
					level* new_level = load_level(levels[index]);
					if(editor_) {
						new_level->set_editor();
					}

					new_level->set_as_current_level();

					if(!new_level->music().empty()) {
						sound::play_music(new_level->music());
					}

					set_scene_title(new_level->title());
					lvl_.reset(new_level);

#ifndef NO_EDITOR
					if(editor_) {
						editor_ = editor::get_editor(lvl_->id().c_str());
						editor_->set_playing_level(lvl_);
						editor_->setup_for_editing();
						lvl_->set_as_current_level();
						lvl_->set_editor();
						init_history_slider();
					}
#endif
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
#ifndef NO_EDITOR
					init_history_slider();
#endif
					if(!paused) {
						controls::read_until(lvl_->cycle());
					}
				} else if(key == SDLK_p && mod & KMOD_ALT) {
					preferences::set_use_pretty_scaling(!preferences::use_pretty_scaling());
					graphics::texture::clear_textures();
				} else if(key == SDLK_f && mod & KMOD_CTRL) {
					preferences::set_fullscreen(!preferences::fullscreen());
					graphics::set_video_mode(graphics::screen_width(), graphics::screen_height());
				}
				break;
			}
#if defined(__ANDROID__)
            case SDL_JOYBUTTONUP:
            case SDL_JOYBUTTONDOWN:
            case SDL_JOYBALLMOTION:
                iphone_controls::handle_event(event);
				handle_mouse_events(event);
                break;
#elif defined(TARGET_OS_HARMATTAN) || defined(TARGET_BLACKBERRY)
			case SDL_MOUSEMOTION:
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
				iphone_controls::handle_event(event);
				handle_mouse_events(event);
				break;
#else
#ifndef NO_EDITOR
			case SDL_MOUSEBUTTONDOWN:
				if(console_.get()) {
					int mousex, mousey;
					SDL_GetMouseState(&mousex, &mousey);
					entity_ptr selected = lvl_->get_next_character_at_point(last_draw_position().x/100 + mousex, last_draw_position().y/100 + mousey, last_draw_position().x/100, last_draw_position().y/100);
					if(selected) {
						lvl_->set_editor_highlight(selected);
						console_->set_focus(selected);
					}
				} else {
					handle_mouse_events(event);
				}
				break;

			case SDL_MOUSEMOTION:
			case SDL_MOUSEBUTTONUP:
				handle_mouse_events(event);
				break;
#endif // NO_EDITOR
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
				debug_console::process_graph();
				lvl_->process();
			} catch(interrupt_game_exception& e) {
				handle_pause_game_result(e.result);
			}

			const int process_time = SDL_GetTicks() - start_process;
			next_process_ += process_time;
			current_perf.process = process_time;
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
		bool should_draw = true;
		
		if(editor_ && paused) {
#ifndef NO_EDITOR
			const int xpos = editor_->xpos();
			const int ypos = editor_->ypos();
			editor_->handle_scrolling();
			last_draw_position().x += (editor_->xpos() - xpos)*100;
			last_draw_position().y += (editor_->ypos() - ypos)*100;

			float target_zoom = 1.0/editor_->zoom();
			float diff = target_zoom - last_draw_position().zoom;
			float amount = diff/10.0;
			float dir = amount > 0.0 ? 1.0 : -1.0;
			if(amount*dir < 0.02) {
				amount = 0.02*dir;
			}

			if(amount*dir > diff*dir) {
				amount = diff;
			}
			last_draw_position().zoom += amount;
#endif
		} else {
			should_draw = update_camera_position(*lvl_, last_draw_position(), NULL, !is_skipping_game());
		}

		lvl_->process_draw();

		if(should_draw) {
#ifndef NO_EDITOR
			if(editor_ && key[SDLK_l] && !editor_->has_keyboard_focus() && (!console_ || !console_->has_keyboard_focus())) {

				editor_->toggle_active_level();

				render_scene(editor_->get_level(), last_draw_position(), NULL, !is_skipping_game());

				editor_->toggle_active_level();
				lvl_->set_as_current_level();
			} else {
				std::vector<variant> alpha_values;
				if(!history_trails_.empty()) {
					foreach(entity_ptr e, history_trails_) {
						alpha_values.push_back(e->query_value("alpha"));
						e->mutate_value("alpha", variant(32));
						lvl_->add_draw_character(e);
					}
				}
#endif
				render_scene(*lvl_, last_draw_position(), NULL, !is_skipping_game());
#ifndef NO_EDITOR
				int index = 0;
				if(!history_trails_.empty()) {
					foreach(entity_ptr e, history_trails_) {
						e->mutate_value("alpha", alpha_values[index++]);
					}

					lvl_->set_active_chars();
				}
			}

			if(editor_) {
				editor_->draw_gui();
			}

			if(history_slider_ && paused) {
				history_slider_->draw();
				history_button_->draw();
			}

			if(console_) {
				console_->draw();
			}
#endif
		}

		performance_data perf(current_fps_, current_cycles_, current_delay_, current_draw_, current_process_, current_flip_, cycle, current_events_, profiling_summary_);

#if TARGET_IPHONE_SIMULATOR || TARGET_OS_HARMATTAN || TARGET_OS_IPHONE
		if( ! is_achievement_displayed() ){
			settings_dialog.draw(in_speech_dialog());
		}
#endif
		
		if(!is_skipping_game() && preferences::show_fps()) {
			draw_fps(*lvl_, perf);
		}

		const int draw_time = SDL_GetTicks() - start_draw;
		next_draw_ += draw_time;
		current_perf.draw = draw_time;

		const int start_flip = SDL_GetTicks();
		if(!is_skipping_game()) {
			SDL_GL_SwapBuffers();
#if defined(__ANDROID__)
			graphics::reset_opengl_state();
#endif
		}

		const int flip_time = SDL_GetTicks() - start_flip;
		next_flip_ += flip_time;
		current_perf.flip = flip_time;
		++next_fps_;
		nskip_draw_ = 0;
	} else {
		++nskip_draw_;
	}

	++next_cycles_;
	current_perf.cycle = next_cycles_;

	static int prev_events_per_second = 0;
	current_perf.nevents = custom_object::events_handled_per_second - prev_events_per_second;
	prev_events_per_second = custom_object::events_handled_per_second;

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
		prev_events_per_second = custom_object::events_handled_per_second = 0;

		profiling_summary_ = formula_profiler::get_profile_summary();
	}

	const int raw_wait_time = desired_end_time - SDL_GetTicks();
	const int wait_time = std::max<int>(1, desired_end_time - SDL_GetTicks());
	next_delay_ += wait_time;
	current_perf.delay = wait_time;
	if (wait_time != 1 && !is_skipping_game()) {
		SDL_Delay(wait_time);
	}

	performance_data::set_current(current_perf);
	
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

void level_runner::toggle_pause()
{
	paused = !paused;
#ifndef NO_EDITOR
	init_history_slider();
#endif
	if(!paused) {
		controls::read_until(lvl_->cycle());
	}
}

void level_runner::reverse_cycle()
{
	const int begin_time = SDL_GetTicks();
	lvl_->reverse_one_cycle();
	lvl_->set_active_chars();
	lvl_->process_draw();

	//remove the control history
	controls::unread_local_controls();

	SDL_Event event;
	while(SDL_PollEvent(&event)) {
	}

	const bool should_draw = update_camera_position(*lvl_, last_draw_position(), NULL, !is_skipping_game());
	render_scene(*lvl_, last_draw_position(), NULL, !is_skipping_game());
	SDL_GL_SwapBuffers();

	const int wait_time = begin_time + 20 - SDL_GetTicks();
	if(wait_time > 0) {
		SDL_Delay(wait_time);
	}
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

#ifndef NO_EDITOR
void level_runner::init_history_slider()
{
	if(paused && editor_) {
		history_slider_.reset(new gui::slider(110, boost::bind(&level_runner::on_history_change, this, _1)));
		history_slider_->set_loc(370, 4);
		history_slider_->set_position(1.0);
		history_button_.reset(new gui::button("Trails", boost::bind(&level_runner::toggle_history_trails, this)));
		history_button_->set_loc(history_slider_->x() + history_slider_->width(), history_slider_->y());
	} else {
		history_slider_.reset();
		history_button_.reset();
		history_trails_.clear();
	}
}

void level_runner::on_history_change(double value)
{
	const int first_frame = lvl_->earliest_backup_cycle();
	const int last_frame = controls::local_controls_end();
	int target_frame = first_frame + (last_frame + 1 - first_frame)*value;
	if(target_frame > last_frame) {
		target_frame = last_frame;
	}

	std::cerr << "TARGET FRAME: " << target_frame << " IN [" << first_frame << ", " << last_frame << "]\n";

	if(target_frame < lvl_->cycle()) {
		lvl_->reverse_to_cycle(target_frame);
	} else if(target_frame > lvl_->cycle()) {
		std::cerr << "STEPPING FORWARD FROM " << lvl_->cycle() << " TO " << target_frame << " /" << controls::local_controls_end() << "\n";

		const controls::control_backup_scope ctrl_scope;
		
		while(lvl_->cycle() < target_frame) {
			lvl_->process();
			lvl_->process_draw();
			lvl_->backup();
		}
	}

	lvl_->set_active_chars();
}

void level_runner::toggle_history_trails()
{
	if(history_trails_.empty() && lvl_->player()) {
		update_history_trails();
	} else {
		history_trails_.clear();
		history_trails_label_.clear();
	}
}

void level_runner::update_history_trails()
{
	entity_ptr e;
	if(history_trails_label_.empty() == false && lvl_->get_entity_by_label(history_trails_label_)) {
		e = lvl_->get_entity_by_label(history_trails_label_);
	} else if(lvl_->editor_selection().empty() == false) {
		e = lvl_->editor_selection().front();
	} else if(lvl_->player()) {
		e = entity_ptr(&lvl_->player()->get_entity());
	}

	if(e) {
		const int first_frame = lvl_->earliest_backup_cycle();
		const int last_frame = controls::local_controls_end();

		const int ncycles = (last_frame - first_frame) + 1;
		history_trails_ = lvl_->predict_future(e, ncycles);
		history_trails_state_id_ = editor_->level_state_id();
		object_reloads_state_id_ = custom_object_type::num_object_reloads();
		tile_rebuild_state_id_ = level::tile_rebuild_state_id();

		history_trails_label_ = e->label();
	}
}
#endif
