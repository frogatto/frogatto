#include <boost/bind.hpp>
#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <time.h>

#include "achievements.hpp"
#include "asserts.hpp"
#include "blur.hpp"
#include "clipboard.hpp"
#include "collision_utils.hpp"
#include "controls.hpp"
#include "current_generator.hpp"
#include "custom_object_functions.hpp"
#include "custom_object.hpp"
#include "debug_console.hpp"
#include "draw_scene.hpp"
#include "entity.hpp"
#include "filesystem.hpp"
#include "formatter.hpp"
#include "formula_callable_definition.hpp"
#include "formula_function_registry.hpp"
#include "formula_profiler.hpp"
#include "i18n.hpp"
#include "json_parser.hpp"
#include "level.hpp"
#include "level_runner.hpp"
#include "load_level.hpp"
#include "object_events.hpp"
#include "pause_game_dialog.hpp"
#include "player_info.hpp"
#include "raster.hpp"
#if !defined(__native_client__)
#include "tbs_client.hpp"
#endif
#include "texture.hpp"
#include "message_dialog.hpp"
#include "options_dialog.hpp"
#include "playable_custom_object.hpp"
#include "preferences.hpp"
#include "random.hpp"
#include "raster_distortion.hpp"
#include "rectangle_rotator.hpp"
#include "sound.hpp"
#include "speech_dialog.hpp"
#include "stats.hpp"
#include "string_utils.hpp"
#include "thread.hpp"
#include "unit_test.hpp"
#include "preferences.hpp"
#include "settings_dialog.hpp"
#include "module.hpp"
#include "widget_factory.hpp"

using namespace game_logic;

namespace {

const std::string FunctionModule = "custom_object";

FUNCTION_DEF(time, 0, 0, "time() -> timestamp: returns the current real time")
	formula::fail_if_static_context();
	time_t t1;
	time(&t1);
	return variant(static_cast<int>(t1));
END_FUNCTION_DEF(time)

FUNCTION_DEF(translate, 1, 1, "translate(str): returns the translated version of the given string")
	return variant(i18n::tr(args()[0]->evaluate(variables).as_string()));
END_FUNCTION_DEF(translate)

FUNCTION_DEF(performance, 0, 0, "performance(): returns an object with current performance stats")
	formula::fail_if_static_context();
	return variant(performance_data::current());
END_FUNCTION_DEF(performance)

FUNCTION_DEF(get_clipboard_text, 0, 0, "get_clipboard_text(): returns the text currentl in the windowing clipboard")
	formula::fail_if_static_context();
	return variant(copy_from_clipboard(false));
END_FUNCTION_DEF(get_clipboard_text)

class set_clipboard_text_command : public game_logic::command_callable
{
	std::string str_;
public:
	explicit set_clipboard_text_command(const std::string& str) : str_(str)
	{}

	virtual void execute(game_logic::formula_callable& ob) const {
		copy_to_clipboard(str_, false);
	}
};

FUNCTION_DEF(set_clipboard_text, 1, 1, "set_clipboard_text(str): sets the clipboard text to the given string")
	return variant(new set_clipboard_text_command(args()[0]->evaluate(variables).as_string()));
END_FUNCTION_DEF(set_clipboard_text)

#if !defined(__native_client__)
FUNCTION_DEF(tbs_client, 2, 3, "tbs_client(host, port, session=-1): creates a client object to the tbs server")
	const std::string host = args()[0]->evaluate(variables).as_string();
	const int port = args()[1]->evaluate(variables).as_int();
	const int session = args().size() >= 3 ? args()[2]->evaluate(variables).as_int() : -1;

	return variant(new tbs::client(host, formatter() << port, session));
END_FUNCTION_DEF(tbs_client)


void tbs_send_event(entity_ptr e, game_logic::map_formula_callable_ptr callable, const std::string& ev)
{
	e->handle_event(ev, callable.get());
}

class tbs_send_command : public entity_command_callable
{
	variant client_, msg_;
public:
	tbs_send_command(variant client, variant msg) : client_(client), msg_(msg)
	{}

	virtual void execute(level& lvl, entity& ob) const {
		tbs::client* tbs_client = client_.try_convert<tbs::client>();
		game_logic::map_formula_callable_ptr callable(new game_logic::map_formula_callable);
		tbs_client->send_request(msg_.write_json(), callable, boost::bind(tbs_send_event, entity_ptr(&ob), callable, _1));
	}

};

FUNCTION_DEF(tbs_send, 2, 2, "tbs_send(tbs_client, msg): sends a message through the given tbs_client connection")
	variant client = args()[0]->evaluate(variables);
	variant msg = args()[1]->evaluate(variables);

	return variant(new tbs_send_command(client, msg));
END_FUNCTION_DEF(tbs_send)

class tbs_process_command : public entity_command_callable
{
	variant client_;
public:
	explicit tbs_process_command(variant client) : client_(client)
	{}

	virtual void execute(level& lvl, entity& ob) const {
		tbs::client* tbs_client = client_.try_convert<tbs::client>();
		tbs_client->process();
	}
};

FUNCTION_DEF(tbs_process, 1, 1, "tbs_process(tbs_client): processes events for the tbs client")
	variant client = args()[0]->evaluate(variables);
	return variant(new tbs_process_command(client));
END_FUNCTION_DEF(tbs_process)
#endif // __native_client__

class report_command : public entity_command_callable
{
	variant v_;
public:
	explicit report_command(variant v) : v_(v)
	{}

	virtual void execute(level& lvl, entity& ob) const {
		stats::record(v_);
	}
};
	
FUNCTION_DEF(report, 1, 1, "report(): Write a key and a value into [custom] in the stats.")
		return variant(new report_command(args()[0]->evaluate(variables)));
END_FUNCTION_DEF(report)

namespace {
int show_simple_option_dialog(level& lvl, const std::string& text, const std::vector<std::string>& options)
{
	std::vector<std::string> txt;
	txt.push_back(text);
	boost::shared_ptr<speech_dialog> d(new speech_dialog);
	d->set_text(txt);
	d->set_options(options);

	lvl.add_speech_dialog(d);

	bool done = false;
	while(!done) {
		SDL_Event event;
		while(SDL_PollEvent(&event)) {
			if(d->key_press(event)) {
				done = true;
			}
		}
		
		if(d->process() || d->detect_joystick_press()) {
			done = true;
		}

		draw_scene(lvl, last_draw_position(), &lvl.player()->get_entity());
		SDL_GL_SwapBuffers();
#if defined(__ANDROID__)
		graphics::reset_opengl_state();
#endif
		SDL_Delay(20);
	}

	lvl.remove_speech_dialog();

	return d->option_selected();
}
}

class set_save_slot_command : public entity_command_callable
{
public:
	explicit set_save_slot_command(int slot) : slot_(slot)
	{}

	virtual void execute(level& lvl, entity& ob) const {
		bool has_options = false;
		std::vector<std::string> options;
		std::string SaveFiles[] = {"save.cfg", "save2.cfg", "save3.cfg", "save4.cfg"};
		int nslot = 0;
		foreach(const std::string& fname, SaveFiles) {
			options.push_back(formatter() << "Slot " << (nslot+1) << ": (empty)");
			const std::string path = std::string(preferences::user_data_path()) + "/" + fname;
			if(sys::file_exists(path)) {
				has_options = true;
				try {
					const variant doc = json::parse_from_file(path);
					if(doc.is_null() == false) {
						options.back() = formatter() << "Slot " << (nslot+1) << ": " << doc["title"].as_string();
					}
				} catch(json::parse_error&) {
				}
			}

			++nslot;
		}

		if(slot_ >= 0) {
			std::cerr << "setting save slot: " << slot_ << " -> " << SaveFiles[slot_] << "\n";
			preferences::set_save_slot(SaveFiles[slot_]);
		} else if(has_options) {
			const int noption = show_simple_option_dialog(lvl, _("Select save slot to use."), options);
			if(noption != -1) {
				std::cerr << "setting save slot: " << noption << " -> " << SaveFiles[noption] << "\n";
				preferences::set_save_slot(SaveFiles[noption]);
			}
		}
	}
	int slot_;
};

FUNCTION_DEF(set_save_slot, 0, 1, "set_save_slot((optional) int slot): Allows the user to select the save slot, if no slot is specified a dialog is displayed.")
	const int slot = (args().size() > 0) ? args()[0]->evaluate(variables).as_int() : -1;
	ASSERT_LOG(slot == -1 || (slot >= 1 && slot <= 4), "Invalid slot specified: " << slot);
	return variant(new set_save_slot_command(slot-1));
END_FUNCTION_DEF(set_save_slot)

class save_game_command : public entity_command_callable
{
	bool persistent_;
public:
	explicit save_game_command(bool persistent) : persistent_(persistent)
	{}

	virtual void execute(level& lvl, entity& ob) const {
		lvl.player()->get_entity().save_game();
		if(persistent_) {
			variant node = lvl.write();
			if(sound::current_music().empty() == false) {
				node = node.add_attr(variant("music"), variant(sound::current_music()));
			}

			sys::write_file(preferences::save_file_path(), node.write_json());
		}
	}
};

FUNCTION_DEF(checkpoint_game, 0, 0, "checkpoint_game(): saves a checkpoint of the game")
	return variant(new save_game_command(false));
END_FUNCTION_DEF(checkpoint_game)

FUNCTION_DEF(get_save_document, 1, 1, "get_save_document(int slot): gets the FFL document for the save in the given slot")
	const int slot = args()[0]->evaluate(variables).as_int();
	std::string fname = "save.cfg";
	if(slot != 0) {
		fname = formatter() << "save" << (slot+1) << ".cfg";
	}
	const std::string path = std::string(preferences::user_data_path()) + "/" + fname;

	try {
		const variant v = json::parse_from_file(path, json::JSON_NO_PREPROCESSOR);
		return v;
	} catch(json::parse_error&) {
		return variant();
	}
END_FUNCTION_DEF(get_save_document)

FUNCTION_DEF(save_game, 0, 0, "save_game(): saves the current game state")
	return variant(new save_game_command(true));
END_FUNCTION_DEF(save_game)

class load_game_command : public entity_command_callable
{
	std::string transition_;
public:
	explicit load_game_command(const std::string& transition) : transition_(transition) {}
	virtual void execute(level& lvl, entity& ob) const {
		std::vector<std::string> save_options;
		save_options.push_back("save.cfg");

		for(int n = 2; n <= 4; ++n) {
			std::string fname = formatter() << "save" << n << ".cfg";
			if(sys::file_exists(std::string(preferences::user_data_path()) + "/" + fname)) {
				save_options.push_back(fname);
			}
		}

		int noption = 0;

		if(save_options.size() > 1) {
			int nslot = 1;
			std::vector<std::string> option_descriptions;
			foreach(const std::string& option, save_options) {
				const std::string fname = std::string(preferences::user_data_path()) + "/" + option;
				try {
					const variant doc = json::parse_from_file(fname);
					option_descriptions.push_back(formatter() << "Slot " << nslot << ": " << doc["title"].as_string());
				} catch(json::parse_error&) {
					option_descriptions.push_back(formatter() << "Slot " << nslot << ": Frogatto");
				}

				++nslot;
			}

			noption = show_simple_option_dialog(lvl, _("Select save slot to load."), option_descriptions);

			if(noption == -1) {
				return;
			}

			preferences::set_save_slot(save_options[noption]);
		}

		level::portal p;
		p.level_dest = save_options[noption];
		p.dest_starting_pos = true;
		p.saved_game = true; 
		p.transition = transition_;
		lvl.force_enter_portal(p);
	}
};

FUNCTION_DEF(load_game, 0, 1, "load_game(transition): loads the saved game. If transition (a string) is given, it will use that type of transition.")
	std::string transition;
	if(args().size() >= 1) {
		transition = args()[0]->evaluate(variables).as_string();
	}
	return variant(new load_game_command(transition));
END_FUNCTION_DEF(load_game)

FUNCTION_DEF(can_load_game, 0, 0, "can_load_game(): returns true if there is a saved game available to load")
	return variant(sys::file_exists(preferences::save_file_path()));
END_FUNCTION_DEF(can_load_game)

class move_to_standing_command : public entity_command_callable
{
public:
	virtual void execute(level& lvl, entity& ob) const {
		ob.move_to_standing(lvl);
	}
};

FUNCTION_DEF(move_to_standing, 0, 0, "move_to_standing(): tries to move the object downwards if it's in the air, or upwards if it's in solid space, until it's standing on solid ground.");
	return variant(new move_to_standing_command());
END_FUNCTION_DEF(move_to_standing)

class music_command : public entity_command_callable
	{
	public:
		explicit music_command(const std::string& name, const bool loops)
		: name_(name), loops_(loops)
		{}
		virtual void execute(level& lvl, entity& ob) const {
			if(loops_){
				sound::play_music(name_);
			}else{
				sound::play_music_interrupt(name_);
			}
		}
	private:
		std::string name_;
		bool loops_;
	};

FUNCTION_DEF(music, 1, 1, "music(string id): plays the music file given by 'id' in a loop")
	return variant(new music_command(
									 args()[0]->evaluate(variables).as_string(),
									 true));
END_FUNCTION_DEF(music)

FUNCTION_DEF(music_onetime, 1, 1, "music_onetime(string id): plays the music file given by 'id' once")
	return variant(new music_command(
									 args()[0]->evaluate(variables).as_string(),
									 false));
END_FUNCTION_DEF(music_onetime)
	
class sound_command : public entity_command_callable
{
public:
	explicit sound_command(const std::string& name, const bool loops, float volume)
	  : names_(util::split(name)), loops_(loops), volume_(volume)
	{}
	virtual void execute(level& lvl, entity& ob) const {
		if(loops_){
			if (names_.empty() == false){
				int randomNum = rand()%names_.size();  //like a 1d-size die
				if(names_[randomNum].empty() == false) {
					sound::play_looped(names_[randomNum], &ob, volume_);
				}
			}
			
		}else{
			//std::cerr << "PLAY SOUND COMMAND: " << name_ << "\n";
			if (names_.empty() == false){
				int randomNum = rand()%names_.size();  //like a 1d-size die
				if(names_[randomNum].empty() == false) {
					sound::play(names_[randomNum], &ob, volume_);
				}
			}
		}
	}
private:
	std::vector<std::string> names_;
	bool loops_;
	float volume_;
};

FUNCTION_DEF(sound, 1, 2, "sound(string id, decimal volume): plays the sound file given by 'id'")
	return variant(new sound_command(
									 args()[0]->evaluate(variables).as_string(),
									 false,
									 args().size() > 1 ? args()[1]->evaluate(variables).as_decimal().as_float() : 1.0));
END_FUNCTION_DEF(sound)

FUNCTION_DEF(sound_loop, 1, 2, "sound_loop(string id, decimal volume): plays the sound file given by 'id' in a loop")
	return variant(new sound_command(
									 args()[0]->evaluate(variables).as_string(),
									 true,
									 args().size() > 1 ? args()[1]->evaluate(variables).as_decimal().as_float() : 1.0));
END_FUNCTION_DEF(sound_loop)


class sound_volume_command : public entity_command_callable {
public:
	explicit sound_volume_command(const int volume)
	: volume_(volume)
	{}
	virtual void execute(level& lvl,entity& ob) const {
		//sound::change_volume(&ob, volume_);
		ob.set_sound_volume(volume_);
	}
private:
	std::string name_;
	int volume_;
};

FUNCTION_DEF(sound_volume, 1, 1, "sound_volume(int volume): sets the volume of sound effects")
	return variant(new sound_volume_command(
									 args()[0]->evaluate(variables).as_int()));
END_FUNCTION_DEF(sound_volume)

class stop_sound_command : public entity_command_callable
{
public:
	explicit stop_sound_command(const std::string& name)
	  : name_(name)
	{}
	virtual void execute(level& lvl, entity& ob) const {
		sound::stop_sound(name_, &ob);
	}
private:
	std::string name_;
};

FUNCTION_DEF(stop_sound, 1, 1, "stop_sound(string id): stops the sound that the current object is playing with the given id")
	return variant(new stop_sound_command(
					args()[0]->evaluate(variables).as_string()));
END_FUNCTION_DEF(stop_sound)

class preload_sound_command : public entity_command_callable
{
public:
	explicit preload_sound_command(const std::string& name)
	  : name_(name)
	{}
	virtual void execute(level& lvl, entity& ob) const {
		sound::preload(name_);
	}
private:
	std::string name_;
};

FUNCTION_DEF(preload_sound, 1, 1, "preload_sound(string id): preload the given sound so it'll be in the sound effects cache")
	return variant(new preload_sound_command(
					args()[0]->evaluate(variables).as_string()));
END_FUNCTION_DEF(preload_sound)

class screen_flash_command : public entity_command_callable
{
public:
	screen_flash_command(const graphics::color_transform& color,
	                     const graphics::color_transform& delta, int duration)
	  : color_(color), delta_(delta), duration_(duration)
	{}

	virtual void execute(level& lvl, entity& ob) const {
		screen_color_flash(color_, delta_, duration_);
	}
private:
	graphics::color_transform color_, delta_;
	int duration_;
};

FUNCTION_DEF(screen_flash, 2, 3, "screen_flash(list int[4] color, (optional) list int[4] delta, int duration): flashes the screen the given color, and keeps the flash going for duration cycles. If delta is given, the color of the flash will be changed every cycle until the duration expires.")
	const variant color = args()[0]->evaluate(variables);
	const variant delta = args().size() > 2 ? args()[1]->evaluate(variables) : variant();
	const variant duration = args()[args().size() - 1]->evaluate(variables);
	ASSERT_LOG(color.is_list() && color.num_elements() == 4 &&
	           (delta.is_null() || delta.is_list() && delta.num_elements() == 4) &&
	           duration.is_int(),
	           "BAD ARGUMENT TO screen_flash() FUNCTION: ARGUMENT FORMAT "
			   "IS screen_flash([r,g,b,a], (optional)[dr,dg,db,da], duration)");
	graphics::color_transform delta_color = graphics::color_transform(0,0,0,0);
	if(delta.is_null() == false) {
		delta_color = graphics::color_transform(
		  delta[0].as_int(), delta[1].as_int(),
		  delta[2].as_int(), delta[3].as_int());
	}

	return variant(new screen_flash_command(
	  graphics::color_transform(color[0].as_int(), color[1].as_int(),
	                            color[2].as_int(), color[3].as_int()),
					  delta_color, duration.as_int()));
END_FUNCTION_DEF(screen_flash)

class title_command : public entity_command_callable
{
public:
	title_command(const std::string& title, int duration)
	  : title_(title), duration_(duration)
	{}

	virtual void execute(level& lvl, entity& ob) const {
		set_scene_title(title_, duration_);
	}
private:
	std::string title_;
	int duration_;
};

FUNCTION_DEF(title, 1, 2, "title(string text, int duration=50): shows level title text on the screen for duration cycles")
	return variant(new title_command(
	  args()[0]->evaluate(variables).as_string(),
	  args().size() >= 2 ? args()[1]->evaluate(variables).as_int() : 50));
END_FUNCTION_DEF(title)

class shake_screen_command : public entity_command_callable
	{
	public:
		explicit shake_screen_command(int x_offset, int y_offset, int x_velocity, int y_velocity)
		: x_offset_(x_offset), y_offset_(y_offset), x_velocity_(x_velocity), y_velocity_(y_velocity)
		{}
		virtual void execute(level& lvl, entity& ob) const {
			screen_position& screen_pos = last_draw_position();
			
			screen_pos.shake_x_offset = x_offset_;
			screen_pos.shake_y_offset = y_offset_;
			screen_pos.shake_x_vel = x_velocity_;
			screen_pos.shake_y_vel = y_velocity_;
		}
	private:
		int x_offset_,y_offset_,x_velocity_,y_velocity_;
	};

FUNCTION_DEF(shake_screen, 4, 4, "shake_screen(int x_offset, int y_offset, int x_velocity, int y_velocity): makes the screen camera shake")
	return variant(new shake_screen_command(
									args()[0]->evaluate(variables).as_int(),
									args()[1]->evaluate(variables).as_int(),
									args()[2]->evaluate(variables).as_int(),
									args()[3]->evaluate(variables).as_int() ));
END_FUNCTION_DEF(shake_screen)

FUNCTION_DEF(radial_current, 2, 2, "radial_current(int intensity, int radius) -> current object: creates a current generator with the given intensity and radius")
	return variant(new radial_current_generator(args()[0]->evaluate(variables).as_int(), args()[1]->evaluate(variables).as_int()));
END_FUNCTION_DEF(radial_current)

FUNCTION_DEF(distortion, 3, 3, "distortion(int, int, int): (currently unsupported")
	return variant(new graphics::radial_distortion(
	                       args()[0]->evaluate(variables).as_int(),
	                       args()[1]->evaluate(variables).as_int(),
	                       args()[2]->evaluate(variables).as_int()));
END_FUNCTION_DEF(distortion)

class execute_on_command : public entity_command_callable
{
	entity_ptr e_;
	variant cmd_;
public:
	execute_on_command(entity_ptr e, variant cmd) : e_(e), cmd_(cmd)
	{}

	virtual void execute(level& lvl, entity& ob) const {
		e_->execute_command(cmd_);
	}
};

FUNCTION_DEF(execute, 2, 2, "execute(object context, command cmd): this function will execute the command or list of commands given by cmd on the object given by context. For instance, animation('foo') will set the current object to animation 'foo'. execute(obj, animation('foo')) can be used to set the object given by obj to the animation 'foo'.")
	entity_ptr e(args()[0]->evaluate(variables).convert_to<entity>());
	variant cmd = args()[1]->evaluate(variables);
	return variant(new execute_on_command(e, cmd));
END_FUNCTION_DEF(execute)

class spawn_command : public entity_command_callable
{
public:
	spawn_command(boost::intrusive_ptr<custom_object> obj, variant instantiation_commands)
	  : obj_(obj), instantiation_commands_(instantiation_commands)
	{}
	virtual void execute(level& lvl, entity& ob) const {
		obj_->set_level(lvl);
		obj_->set_spawned_by(ob.label());

		if(!place_entity_in_level_with_large_displacement(lvl, *obj_)) {
			return;
		}

		lvl.add_character(obj_);

		obj_->execute_command(instantiation_commands_);

		//send an event to the parent to let them know they've spawned a child,
		//and let them record the child's details.
		game_logic::map_formula_callable* spawn_callable(new game_logic::map_formula_callable);
		variant holder(spawn_callable);
		spawn_callable->add("spawner", variant(&ob));
		spawn_callable->add("child", variant(obj_.get()));
		ob.handle_event("child_spawned", spawn_callable);
		obj_->handle_event("spawned", spawn_callable);
		obj_->create_object();

		if(entity_collides(lvl, *obj_, MOVE_NONE)) {
			lvl.remove_character(obj_);
		}
	}
private:
	boost::intrusive_ptr<custom_object> obj_;
	variant instantiation_commands_;
};

FUNCTION_DEF(spawn, 4, 5, "spawn(string type_id, int midpoint_x, int midpoint_y, int facing, (optional) list of commands cmd): will create a new object of type given by type_id with the given midpoint and facing. Immediately after creation the object will have any commands given by cmd executed on it. The child object will have the spawned event sent to it, and the parent object will have the child_spawned event sent to it.")

	formula::fail_if_static_context();

	const std::string type = EVAL_ARG(0).as_string();
	const int x = EVAL_ARG(1).as_int();
	const int y = EVAL_ARG(2).as_int();
	const bool facing = EVAL_ARG(3).as_int() > 0;
	boost::intrusive_ptr<custom_object> obj(new custom_object(type, x, y, facing));
	obj->set_pos(obj->x() - obj->current_frame().width() / 2 , obj->y() - obj->current_frame().height() / 2);

	variant commands;
	if(args().size() > 4) {
		map_formula_callable_ptr callable = new map_formula_callable(&variables);
		callable->add("child", variant(obj.get()));
		commands = args()[4]->evaluate(*callable);
	}

	return variant(new spawn_command(obj, commands));
END_FUNCTION_DEF(spawn)

FUNCTION_DEF(spawn_player, 4, 5, "spawn_player(string type_id, int midpoint_x, int midpoint_y, int facing, (optional) list of commands cmd): identical to spawn except that the new object is playable.")

	formula::fail_if_static_context();

	const std::string type = EVAL_ARG(0).as_string();
	const int x = EVAL_ARG(1).as_int();
	const int y = EVAL_ARG(2).as_int();
	const bool facing = EVAL_ARG(3).as_int() > 0;
	boost::intrusive_ptr<custom_object> obj(new custom_object(type, x, y, facing));
	obj.reset(new playable_custom_object(*obj));
	obj->set_pos(obj->x() - obj->current_frame().width() / 2 , obj->y() - obj->current_frame().height() / 2);

	variant commands;
	if(args().size() > 4) {
		map_formula_callable_ptr callable = new map_formula_callable(&variables);
		callable->add("child", variant(obj.get()));
		commands = args()[4]->evaluate(*callable);
	}

	return variant(new spawn_command(obj, commands));

END_FUNCTION_DEF(spawn_player)

FUNCTION_DEF(object, 1, 5, "object(string type_id, int midpoint_x, int midpoint_y, int facing, (optional) map properties) -> object: constructs and returns a new object. Note that the difference between this and spawn is that spawn returns a command to actually place the object in the level. object only creates the object and returns it. It may be stored for later use.")
	formula::fail_if_static_context();
	const std::string type = args()[0]->evaluate(variables).as_string();
	custom_object* obj;
	
	if(args().size() > 1) {
		const int x = args()[1]->evaluate(variables).as_int();
		const int y = args()[2]->evaluate(variables).as_int();
		const bool face_right = args()[3]->evaluate(variables).as_int() > 0;
		obj = new custom_object(type, x, y, face_right);
	} else {
		const int x = 0;
		const int y = 0;
		const bool face_right = true;
		obj = new custom_object(type, x, y, face_right);
	}
		
	//adjust so the object's x/y is its midpoint.
	obj->set_pos(obj->x() - obj->current_frame().width() / 2 , obj->y() - obj->current_frame().height() / 2);

	if(args().size() > 4) {
		variant properties = args()[4]->evaluate(variables);
		variant keys = properties.get_keys();
		for(int n = 0; n != keys.num_elements(); ++n) {
			variant value = properties[keys[n]];
			obj->mutate_value(keys[n].as_string(), value);
		}
	}

	return variant(obj);
END_FUNCTION_DEF(object)

FUNCTION_DEF(object_playable, 1, 5, "object_playable(string type_id, int midpoint_x, int midpoint_y, int facing, (optional) map properties) -> object: constructs and returns a new object. Note that the difference between this and spawn is that spawn returns a command to actually place the object in the level. object_playable only creates the playble object and returns it. It may be stored for later use.")
	formula::fail_if_static_context();
	const std::string type = args()[0]->evaluate(variables).as_string();
	playable_custom_object* obj;
	
	if(args().size() > 1) {
		const int x = args()[1]->evaluate(variables).as_int();
		const int y = args()[2]->evaluate(variables).as_int();
		const bool face_right = args()[3]->evaluate(variables).as_int() > 0;
		obj = new playable_custom_object(custom_object(type, x, y, face_right));
	} else {
		const int x = 0;
		const int y = 0;
		const bool face_right = true;
		obj = new playable_custom_object(custom_object(type, x, y, face_right));
	}
		
	//adjust so the object's x/y is its midpoint.
	obj->set_pos(obj->x() - obj->current_frame().width() / 2 , obj->y() - obj->current_frame().height() / 2);

	if(args().size() > 4) {
		variant properties = args()[4]->evaluate(variables);
		variant keys = properties.get_keys();
		for(int n = 0; n != keys.num_elements(); ++n) {
			variant value = properties[keys[n]];
			obj->mutate_value(keys[n].as_string(), value);
		}
	}

	return variant(obj);
END_FUNCTION_DEF(object_playable)

class animation_command : public custom_object_command_callable
{
public:
	animation_command(const std::string& anim)
	  : anim_(anim)
	{}

	virtual void execute(level& lvl, custom_object& ob) const {
		ob.set_frame(anim_);
	}
private:
	std::string anim_;
};

FUNCTION_DEF(animation, 1, 1, "animation(string id): changes the current object's animation to the given animation. time_in_animation is reset to 0.")
	return variant(new animation_command(args()[0]->evaluate(variables).as_string()));
END_FUNCTION_DEF(animation)

class die_command : public custom_object_command_callable
{
public:
	virtual void execute(level& lvl, custom_object& ob) const {
		ob.die();
	}
}; 

FUNCTION_DEF(die, 0, 0, "die(): causes the current object to die. The object will receive the on_die signal and may even use it to resurrect itself. Use remove_object() to remove an object from play without it receiving on_die.")
	return variant(new die_command());
END_FUNCTION_DEF(die)

class facing_command : public custom_object_command_callable
{
public:
	explicit facing_command(int facing) : facing_(facing)
	{}
	virtual void execute(level& lvl, custom_object& ob) const {
		ob.set_face_right(facing_ > 0);
	}
private:
	int facing_;
};

FUNCTION_DEF(facing, 1, 1, "facing(int new_facing): changes the current object's facing according to the value of new_facing (1 for right, otherwise left).")
	return variant(new facing_command(args()[0]->evaluate(variables).as_int()));
END_FUNCTION_DEF(facing)

class set_var_command : public custom_object_command_callable
{
public:
	explicit set_var_command(const std::string& attr, variant val)
	  : attr_(attr), val_(val)
	{}
	virtual void execute(level& lvl, custom_object& ob) const {
		ob.vars()->mutate_value(attr_, val_);
	}
private:
	std::string attr_;
	variant val_;
};

FUNCTION_DEF(set_var, 2, 2, "set_var(string varname, variant value): sets the variable named varname within the current object. Note that you should generally use set(vars.blah, x) rather than set_var('blah', x). The only exception is if you want to create the command and save it for later execution on an object you don't yet have access to -- most useful with the spawn() function.")
	return variant(new set_var_command(
	    args()[0]->evaluate(variables).as_string(),
		args()[1]->evaluate(variables)));
END_FUNCTION_DEF(set_var)

class add_debug_chart_command : public game_logic::command_callable {
	std::string id_;
	decimal value_;
public:
	add_debug_chart_command(const std::string& id, decimal value)
	  : id_(id), value_(value)
	{}

	virtual void execute(game_logic::formula_callable& ob) const {
	debug_console::add_graph_sample(id_, value_);
	}
};

FUNCTION_DEF(debug_chart, 2, 2, "debug_chart(string id, decimal value): plots a sample in a graph")
	return variant(new add_debug_chart_command(args()[0]->evaluate(variables).as_string(), args()[1]->evaluate(variables).as_decimal()));
END_FUNCTION_DEF(debug_chart)

class add_debug_rect_command : public game_logic::command_callable {
	rect r_;
public:
	explicit add_debug_rect_command(const rect& r) : r_(r)
	{}

	virtual void execute(game_logic::formula_callable& ob) const {
		add_debug_rect(r_);
	}
};

FUNCTION_DEF(solid, 3, 6, "solid(level, int x, int y, (optional)int w=1, (optional) int h=1, (optional) int debug=0) -> boolean: returns true iff the level contains solid space within the given (x,y,w,h) rectangle. If 'debug' is set, then the tested area will be displayed on-screen.")
	level* lvl = args()[0]->evaluate(variables).convert_to<level>();
	const int x = args()[1]->evaluate(variables).as_int();
	const int y = args()[2]->evaluate(variables).as_int();

	int w = args().size() >= 4 ? args()[3]->evaluate(variables).as_int() : 1;
	int h = args().size() >= 5 ? args()[4]->evaluate(variables).as_int() : 1;

	rect r(x, y, w, h);

	if(args().size() >= 6) {
		//debugging so set the debug rect
		add_debug_rect(r);
	}

	return variant(lvl->solid(r));
END_FUNCTION_DEF(solid)

FUNCTION_DEF(debug_rect, 2, 5, "debug_rect(int x, int y, (optional)int w=1, (optional) int h=1) -> Draws, for one frame, a rectangle on the level")
	const int x = args()[0]->evaluate(variables).as_int();
	const int y = args()[1]->evaluate(variables).as_int();

	int w = args().size() >= 3 ? args()[2]->evaluate(variables).as_int() : 100;
	int h = args().size() >= 4 ? args()[3]->evaluate(variables).as_int() : 100;

	rect r(x, y, w, h);
	return variant(new add_debug_rect_command(r));
END_FUNCTION_DEF(debug_rect)

FUNCTION_DEF(plot_x, 1, 1, "plot_x(int x): plots a vertical debug line at the given position")
	const int x = args()[0]->evaluate(variables).as_int();
	return variant(new add_debug_rect_command(rect(x, -32000, 2, 64000)));
END_FUNCTION_DEF(plot_x)

FUNCTION_DEF(plot_y, 1, 1, "plot_y(int x): plots a horizontal debug line at the given position")
	const int y = args()[0]->evaluate(variables).as_int();
	return variant(new add_debug_rect_command(rect(-32000, y, 64000, 2)));
END_FUNCTION_DEF(plot_y)

FUNCTION_DEF(standable, 3, 5, "standable(level, int x, int y, (optional)int w=1, (optional) int h=1) -> boolean: returns true iff the level contains standable space within the given (x,y,w,h) rectangle")
	level* lvl = args()[0]->evaluate(variables).convert_to<level>();
	const int x = args()[1]->evaluate(variables).as_int();
	const int y = args()[2]->evaluate(variables).as_int();

	int w = args().size() >= 4 ? args()[3]->evaluate(variables).as_int() : 1;
	int h = args().size() >= 5 ? args()[4]->evaluate(variables).as_int() : 1;

	rect r(x, y, w, h);

	return variant(lvl->standable(r));
END_FUNCTION_DEF(standable)

class set_solid_command : public entity_command_callable {
	rect r_;
	bool is_solid_;
public:
	set_solid_command(const rect& r, bool is_solid) : r_(r), is_solid_(is_solid)
	{}

	virtual void execute(level& lvl, entity& ob) const {
		lvl.set_solid_area(r_, is_solid_);
	}
};

FUNCTION_DEF(set_solid, 4, 5, "set_solid(x1, y1, x2, y2, boolean is_solid=false): modifies the solidity of the level such that the rectangle given by (x1, y1, x2, y2) will have its solidity set to the value of is_solid")
	return variant(new set_solid_command(
	  rect::from_coordinates(
		args()[0]->evaluate(variables).as_int(),
		args()[1]->evaluate(variables).as_int(),
		args()[2]->evaluate(variables).as_int(),
		args()[3]->evaluate(variables).as_int()),
		args().size() > 4 ? args()[4]->evaluate(variables).as_bool() : false));

END_FUNCTION_DEF(set_solid)

FUNCTION_DEF(group_size, 2, 2, "group_size(level, int group_id) -> int: gives the number of objects in the object group given by group_id")
	level* lvl = args()[0]->evaluate(variables).convert_to<level>();
	return variant(lvl->group_size(args()[1]->evaluate(variables).as_int()));
END_FUNCTION_DEF(group_size)

class set_group_command : public entity_command_callable {
public:
	explicit set_group_command(int group=-1) : group_(group)
	{}

	virtual void execute(level& lvl, entity& ob) const {
		int group = group_;
		if(group < 0) {
			if(ob.group() >= 0) {
				return;
			}
			group = lvl.add_group();
		}

		lvl.set_character_group(&ob, group);
	}

private:
	int group_;
};

FUNCTION_DEF(set_group, 0, 1, "set_group((optional)int group_id): sets the current object to have the given group id, or to be in no group if group_id is not given")
	int group = -1;
	if(args().size() > 0) {
		group = args()[0]->evaluate(variables).as_int();
	}

	return variant(new set_group_command(group));
END_FUNCTION_DEF(set_group)

class scroll_to_command : public custom_object_command_callable
{
public:
	explicit scroll_to_command(entity_ptr focus) : focus_(focus)
	{}
	virtual void execute(level& lvl, custom_object& ob) const {
		screen_position pos = last_draw_position();
		for(int n = 0; n != 50; ++n) {
			draw_scene(lvl, pos, focus_.get());
			SDL_GL_SwapBuffers();
#if defined(__ANDROID__)
			graphics::reset_opengl_state();
#endif
			SDL_Delay(20);
		}
	}
private:
	entity_ptr focus_;
};

FUNCTION_DEF(tiles_at, 2, 2, "tiles_at(x, y): gives a list of the tiles at the given x, y position")
	formula::fail_if_static_context();

	std::vector<variant> v;

	std::pair<level::TileItor, level::TileItor> range = level::current().tiles_at_loc(args()[0]->evaluate(variables).as_int(), args()[1]->evaluate(variables).as_int());
	while(range.first != range.second) {
		v.push_back(variant(range.first->object));
		++range.first;
	}

	return variant(&v);
END_FUNCTION_DEF(tiles_at)

FUNCTION_DEF(scroll_to, 1, 1, "scroll_to(object target): scrolls the screen to the target object")
	return variant(new scroll_to_command(args()[0]->evaluate(variables).try_convert<entity>()));
END_FUNCTION_DEF(scroll_to)

namespace {
static int g_in_speech_dialog = 0;
class in_speech_dialog_tracker {
public:
	in_speech_dialog_tracker() : cancelled_(false) {
		g_in_speech_dialog++;
	}

	~in_speech_dialog_tracker() {
		cancel();
	}

	void cancel() {
		if(!cancelled_) {
			g_in_speech_dialog--;
			cancelled_ = true;
		}
	}

private:
	bool cancelled_;
};
}

class transient_speech_dialog_command : public custom_object_command_callable {
	entity_ptr speaker_;
	std::vector<std::string> text_;
	int duration_;
public:
	transient_speech_dialog_command(entity_ptr speaker, const std::vector<std::string>& text, int duration) : speaker_(speaker), text_(text), duration_(duration)
	{}
	virtual void execute(level& lvl, custom_object& ob) const {
		boost::shared_ptr<speech_dialog> d(new speech_dialog);
		if(speaker_) {
			d->set_speaker(speaker_);
		} else {
			d->set_speaker(entity_ptr(&ob));
		}

		d->set_text(text_);
		d->set_expiration(duration_);
		lvl.add_speech_dialog(d);
	}
};

FUNCTION_DEF(transient_speech_dialog, 1, -1, "transient_speech_dialog(...): schedules a sequence of speech dialogs to be shown. Arguments may include a list of strings, which contain text. An integer which sets the duration of the dialog. An object which sets the speaker.")
	entity_ptr speaker;
	int duration = 100;

	std::vector<variant> result;

	for(int n = 0; n != args().size(); ++n) {
		variant v = args()[n]->evaluate(variables);
		entity* e = v.try_convert<entity>();
		if(e) {
			speaker = entity_ptr(e);
		} else if(v.is_int()) {
			duration = v.as_int();
		} else if(v.is_list()) {
			std::vector<std::string> str;
			for(int m = 0; m != v.num_elements(); ++m) {
				str.push_back(v[m].as_string());
			}
			result.push_back(variant(new transient_speech_dialog_command(speaker, str, duration)));
		} else {
			ASSERT_LOG(false, "UNRECOGNIZED ARGUMENT to transient_speech_dialog: " << v.to_debug_string());
		}
	}

	//we add the dialogs in reverse order to the level to make it get
	//said in the correct order.
	std::reverse(result.begin(), result.end());

	return variant(&result);
END_FUNCTION_DEF(transient_speech_dialog)

namespace {
//if this variable is set we are in a controlled dialog sequence skipping
//scope and the end of dialog shouldn't automatically end skipping dialog.
bool skipping_dialog_sequence = false;

struct in_dialog_setter {
	bool was_in_dialog_;
	level& lvl_;
	in_dialog_setter(level& lvl) : lvl_(lvl) { 
		was_in_dialog_ = lvl_.in_dialog();
		lvl_.set_in_dialog(true);
	}
	~in_dialog_setter() {
		lvl_.set_in_dialog(was_in_dialog_);
		if(!was_in_dialog_ && !skipping_dialog_sequence) {
			end_skipping_game();
		}
	}
};

struct speech_dialog_scope {
	level& lvl_;
	boost::shared_ptr<speech_dialog> dialog_;

	speech_dialog_scope(level& lvl, boost::shared_ptr<speech_dialog> dialog)
	  : lvl_(lvl), dialog_(dialog)
	{
		lvl_.add_speech_dialog(dialog_);
	}

	~speech_dialog_scope()
	{
		lvl_.remove_speech_dialog();
	}
};

}

class skip_sequence_command : public custom_object_command_callable {
	bool skip_on_;
public:
	explicit skip_sequence_command(bool skip_on)
	  : skip_on_(skip_on)
	{}

	virtual void execute(level& lvl, custom_object& ob) const {
		skipping_dialog_sequence = skip_on_;
		if(!skip_on_) {
			end_skipping_game();
		}
	}
};

FUNCTION_DEF(begin_skip_dialog_sequence, 0, 0, "begin_skip_dialog_sequence(): command that will cause everything up until the next time end_skip_dialog_sequence() is called to be considered a single storyline sequence. If the player selects to skip the sequence between now and then everything up until the call to end_skip_dialog_sequence() will be skipped.")
	return variant(new skip_sequence_command(true));
END_FUNCTION_DEF(begin_skip_dialog_sequence)

FUNCTION_DEF(end_skip_dialog_sequence, 0, 0, "end_skip_dialog_sequence(): ends the sequence begun with begin_skip_dialog_sequence().")
	std::cerr << "ENDDIALOG\n";
	return variant(new skip_sequence_command(false));
END_FUNCTION_DEF(end_skip_dialog_sequence)

class speech_dialog_command : public custom_object_command_callable {
public:
	explicit speech_dialog_command(const std::vector<variant>& args, bool paused=false)
	  : args_(args), paused_(paused)
	{}
	virtual void execute(level& lvl, custom_object& ob) const {
//		pause_scope pauser;

		if(!g_in_speech_dialog) {
			foreach(const entity_ptr& e, lvl.get_chars()) {
				e->handle_event(OBJECT_EVENT_BEGIN_DIALOG);
			}
		}

		formula_profiler::suspend_scope profiler_suspend;
		in_dialog_setter dialog_setter(lvl);

		//make it so the player's controls become locked for the duration of the dialog.
		controls::local_controls_lock controller_lock;

		execute_commands(lvl, ob, args_);
	}

private:
	static settings_dialog menu_button_;
	
	void execute_commands(level& lvl, custom_object& ob, const std::vector<variant>& commands) const {
		menu_button_.reset();
		in_speech_dialog_tracker dialog_tracker;

		boost::shared_ptr<speech_dialog> d(new speech_dialog());
		speech_dialog_scope dialog_scope(lvl, d);

		foreach(variant var, commands) {
			if(var.is_callable()) {
				const_entity_ptr e = var.try_convert<entity>();
				if(e) {
					//std::cerr << "set speaker...\n";
					d->set_speaker_and_flip_side(e);
				}

				const entity_command_callable* entity_cmd = var.try_convert<const entity_command_callable>();
				if(entity_cmd) {
					entity_cmd->execute(lvl, ob);
				}

				const custom_object_command_callable* obj_cmd = var.try_convert<const custom_object_command_callable>();
				if(obj_cmd) {
					obj_cmd->execute(lvl, ob);
				}

				const game_logic::command_callable* callable_cmd = var.try_convert<const game_logic::command_callable>();
				if(callable_cmd) {
					callable_cmd->execute(ob);
				}

			}

			if(var.is_list()) {
				if(var.num_elements() > 0 && var[0].is_callable()) {
					std::vector<variant> cmd;
					for(int n = 0; n != var.num_elements(); ++n) {
						cmd.push_back(var[n]);
					}

					execute_commands(lvl, ob, cmd);
					continue;
				}

				bool is_default = false;
				int default_option = -1;

				std::vector<variant> option_commands;
				std::vector<std::string> options;
				std::vector<std::string> message;
				for(int n = 0; n != var.num_elements(); ++n) {
					if(var[n].is_string() && var[n].as_string() == "default_skip") {
						is_default = true;
					} else if(message.empty() == false && var[n].is_list()) {
						if(is_default) {
							default_option = n;
							is_default = false;
						}

						options.push_back(message.back());
						message.pop_back();
						option_commands.push_back(var[n]);
					} else {
						message.push_back(var[n].as_string());
					}
				}

				d->set_text(message);
				d->set_options(options);

				bool done = false;
				while(!done) {
					if(!paused_) {
						debug_console::process_graph();
						lvl.process();
						lvl.process_draw();
					}

					SDL_Event event;
					while(SDL_PollEvent(&event)) {
#if TARGET_IPHONE_SIMULATOR || TARGET_OS_HARMATTAN || TARGET_OS_IPHONE
						// the user event gets handled the same as pressing escape
						if (menu_button_.handle_event(event))
						{
							event.type = SDL_USEREVENT;
						}
#endif
						switch(event.type) {
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
								if (e.type == SDL_ACTIVEEVENT && e.active.gain == 1)
									break;
							}
						}
						break;
#endif
						case SDL_QUIT:
							throw interrupt_game_exception();
						case SDL_USEREVENT:
						case SDL_KEYDOWN:
							if(event.key.keysym.sym == SDLK_ESCAPE || event.type == SDL_USEREVENT) {
								begin_skipping_game();
								if(default_option != -1) {
									d->set_option_selected(default_option);
								}
								break;
							}

						case SDL_MOUSEBUTTONDOWN:
						case SDL_MOUSEBUTTONUP:
						case SDL_MOUSEMOTION:
							done = done || d->key_press(event);
							break;
						}
					}

					done = done || d->detect_joystick_press();

					if(paused_)
						done = done || d->process();
					else if(!lvl.current_speech_dialog())
						done = true;
					draw(lvl);
				}

				if(options.empty() == false) {
					const int index = d->option_selected();
					if(index >= 0 && index < option_commands.size()) {
						dialog_tracker.cancel();
						ob.execute_command(option_commands[index]);
					}
				}

				d->set_options(std::vector<std::string>());
			}
		}
	}

	void draw(const level& lvl) const {
		draw_scene(lvl, last_draw_position(), &lvl.player()->get_entity());
#if TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE
		menu_button_.draw(in_speech_dialog());
#endif

		SDL_GL_SwapBuffers();
#if defined(__ANDROID__)
		graphics::reset_opengl_state();
#endif
		SDL_Delay(20);
	}

	std::vector<variant> args_;

	bool paused_;
};

settings_dialog speech_dialog_command::menu_button_ = settings_dialog();

FUNCTION_DEF(speech_dialog, 1, -1, "speech_dialog(...): schedules a sequence of speech dialogs to be shown modally. Arguments may include a list of strings, which contain text. An integer which sets the duration of the dialog. An object which sets the speaker. A string by itself indicates an option that should be shown for the player to select from. A string should be followed by a list of commands that will be executed should the player choose that option.")
	std::vector<variant> v;
	for(int n = 0; n != args().size(); ++n) {
		v.push_back(args()[n]->evaluate(variables));
	}

	return variant(new speech_dialog_command(v));
END_FUNCTION_DEF(speech_dialog)

FUNCTION_DEF(paused_speech_dialog, 1, -1, "paused_speech_dialog(...): like speech_dialog(), except the game is paused while the dialog is displayed.")
	std::vector<variant> v;
	for(int n = 0; n != args().size(); ++n) {
		v.push_back(args()[n]->evaluate(variables));
	}

	return variant(new speech_dialog_command(v, true));
END_FUNCTION_DEF(paused_speech_dialog)

class end_game_command : public custom_object_command_callable
{
public:
	virtual void execute(level& lvl, custom_object& ob) const {
		lvl.set_end_game();
	}
};

FUNCTION_DEF(end_game, 0, 0, "end_game(): exits the game")
	return variant(new end_game_command());
END_FUNCTION_DEF(end_game)

class achievement_command : public entity_command_callable
{
public:
	explicit achievement_command(const std::string& str) : str_(str)
	{}

	virtual void execute(level& lvl, entity& ob) const {
		if(attain_achievement(str_)) {
			achievement_ptr a = achievement::get(str_);
			if(a) {
				stats::entry("achievement").add_player_pos().set("achievement", variant(str_));
				sound::play("achievement-attained.wav");
				set_displayed_achievement(a);
			}
		}
	}
private:
	std::string str_;
};

FUNCTION_DEF(achievement, 1, 1, "achievement(id): unlocks the achievement with the given id")
	return variant(new achievement_command(args()[0]->evaluate(variables).as_string()));
END_FUNCTION_DEF(achievement)

static int event_depth = 0;
struct event_depth_scope {
	event_depth_scope() { ++event_depth; }
	~event_depth_scope() { --event_depth; }
};

class fire_event_command : public entity_command_callable {
	const entity_ptr target_;
	const std::string event_;
	const const_formula_callable_ptr callable_;
public:
	fire_event_command(entity_ptr target, const std::string& event, const_formula_callable_ptr callable)
	  : target_(target), event_(event), callable_(callable)
	{}

	virtual void execute(level& lvl, entity& ob) const {
		ASSERT_LOG(event_depth < 1000, "INFINITE (or too deep?) RECURSION FOR EVENT " << event_);
		event_depth_scope scope;
		entity* e = target_ ? target_.get() : &ob;
		e->handle_event(event_, callable_.get());
	}
};

namespace {
const_formula_callable_ptr map_into_callable(variant v) {
	if(v.is_callable()) {
		return const_formula_callable_ptr(v.as_callable());
	} else if(v.is_map()) {
		map_formula_callable* res = new map_formula_callable;
		foreach(const variant_pair& p, v.as_map()) {
			res->add(p.first.as_string(), p.second);
		}

		return const_formula_callable_ptr(res);
	} else {
		return const_formula_callable_ptr();
	}
}
}

FUNCTION_DEF(fire_event, 1, 3, "fire_event((optional) object target, string id, (optional)callable arg): fires the event with the given id. Targets the current object by default, or target if given. Sends arg as the event argument if given")
	entity_ptr target;
	std::string event;
	const_formula_callable_ptr callable;

	if(args().size() == 3) {
		variant v1 = args()[0]->evaluate(variables);
		if(v1.is_null()) {
			return variant();
		}

		target = v1.convert_to<entity>();
		event = args()[1]->evaluate(variables).as_string();
		callable = map_into_callable(args()[2]->evaluate(variables));
	} else if(args().size() == 2) {
		variant v1 = args()[0]->evaluate(variables);
		if(v1.is_null()) {
			return variant();
		}

		variant v2 = args()[1]->evaluate(variables);
		if(v1.is_string()) {
			event = v1.as_string();
			callable = map_into_callable(v2);
		} else {
			target = v1.convert_to<entity>();
			event = v2.as_string();
		}
	} else {
		event = args()[0]->evaluate(variables).as_string();
	}

	return variant(new fire_event_command(target, event, callable));
END_FUNCTION_DEF(fire_event)

FUNCTION_DEF(proto_event, 2, 2, "proto_event(prototype, event_name): for the given prototype, fire the named event. e.g. proto_event('playable', 'process')")
	const std::string proto = args()[0]->evaluate(variables).as_string();
	const std::string event_type = args()[1]->evaluate(variables).as_string();
	const std::string event_name = proto + "_PROTO_" + event_type;
	ASSERT_LOG(event_depth < 100, "Infinite (or too deep?) recursion in proto_event(" << proto << ", " << event_type << ")");
	return variant(new fire_event_command(entity_ptr(), event_name, const_formula_callable_ptr(&variables)));
	
END_FUNCTION_DEF(proto_event)

FUNCTION_DEF(get_object, 2, 2, "get_object(level, string label) -> object: returns the object that is present in the given level that has the given label")

	level* lvl = args()[0]->evaluate(variables).try_convert<level>();
	if(lvl) {
		return variant(lvl->get_entity_by_label(args()[1]->evaluate(variables).as_string()).get());
	}

	return variant();
END_FUNCTION_DEF(get_object)

//a command which moves an object in a given direction enough to resolve
//any solid conflicts.
class resolve_solid_command : public entity_command_callable {
	entity_ptr e_;
	int xdir_, ydir_, max_cycles_;
public:
	resolve_solid_command(entity_ptr e, int xdir, int ydir, int max_cycles) : e_(e), xdir_(xdir), ydir_(ydir), max_cycles_(max_cycles)
	{}

	virtual void execute(level& lvl, entity& ob) const {
		if(xdir_ == 0 && ydir_ == 0 && max_cycles_ == 0) {
			if(!place_entity_in_level_with_large_displacement(lvl, ob)) {
				custom_object* custom_obj = dynamic_cast<custom_object*>(&ob);
				if(custom_obj) {
					//we really couldn't place it despite our best efforts ...
					//killing it is our best choice.
					custom_obj->die();
				}
				return;
			}
		}

		const int start_x = e_->x();
		const int start_y = e_->y();

		int max_cycles = max_cycles_;
		while(entity_collides(lvl, *e_, MOVE_NONE) && max_cycles > 0) {
			e_->set_pos(e_->x() + xdir_, e_->y() + ydir_);
			--max_cycles;
		}

		if(max_cycles == 0) {
			e_->set_pos(start_x, start_y);
		}
	}
};

FUNCTION_DEF(resolve_solid, 1, 4, "resolve_solid(object, int xdir, int ydir, int max_cycles=100): will attempt to move the given object in the direction indicated by xdir/ydir until the object no longer has a solid overlap. Gives up after max_cycles. If called with no arguments other than the object, will try desperately to place the object in the level.")
	entity_ptr e(args()[0]->evaluate(variables).try_convert<entity>());
	if(args().size() == 1) {
		return variant(new resolve_solid_command(e, 0, 0, 0));
	} else if(args().size() == 2) {
		std::cerr << "TWO ARGMENTS ISN'T A SUPPORTED OPTION FOR resolve_solid() CONTINUING AS IF ONE ARGUMENT SUPPLIED\n";
		return variant(new resolve_solid_command(e, 0, 0, 0));
	}

	const int xdir = args()[1]->evaluate(variables).as_int();
	const int ydir = args()[2]->evaluate(variables).as_int();
	const int max_cycles = args().size() > 3 ? args()[3]->evaluate(variables).as_int() : 100;
	if(e) {
		return variant(new resolve_solid_command(e, xdir, ydir, max_cycles));
	} else {
		return variant();
	}
END_FUNCTION_DEF(resolve_solid)

class add_object_command : public entity_command_callable {
	entity_ptr e_;
public:
	explicit add_object_command(entity_ptr e) : e_(e)
	{}

	virtual void execute(level& lvl, entity& ob) const {
		e_->set_spawned_by(ob.label());
		if(place_entity_in_level_with_large_displacement(lvl, *e_)) {
			lvl.add_character(e_);
		} else {
			collision_info collide_info;
			entity_collides(level::current(), *e_, MOVE_NONE, &collide_info);
			game_logic::map_formula_callable* callable(new game_logic::map_formula_callable(this));
			callable->add("collide_with", variant(collide_info.collide_with.get()));

			game_logic::formula_callable_ptr callable_ptr(callable);
			e_->handle_event(OBJECT_EVENT_ADD_OBJECT_FAIL, callable);

			if(!e_->destroyed()) {
				callable->add("object", variant(e_.get()));
				ob.handle_event(OBJECT_EVENT_ADD_OBJECT_FAIL, callable);
			}
		}
	}
};

FUNCTION_DEF(add_object, 1, 1, "add_object(object): inserts the given object into the level. The object should not currently be persent in the level. The position of the object is tweaked to make sure there are no solid overlaps, however if it is not possible to reasonably place the object without a solid overlap, then the object will not be placed and the object and caller will both receive the event add_object_fail.")

	entity_ptr e(args()[0]->evaluate(variables).try_convert<entity>());
	if(e) {
		return variant(new add_object_command(e));
	} else {
		std::cerr << "NOT AN OBJECT!\n";
		return variant();
	}
END_FUNCTION_DEF(add_object)

class remove_object_command : public entity_command_callable {
	entity_ptr e_;
public:
	explicit remove_object_command(entity_ptr e) : e_(e)
	{}

	virtual void execute(level& lvl, entity& ob) const {
		lvl.remove_character(e_);
	}
};

FUNCTION_DEF(remove_object, 1, 1, "remove_object(object): removes the given object from the level. If there are no references to the object stored, then the object will immediately be destroyed. However it is possible to keep a reference to the object and even insert it back into the level later using add_object()")

	entity_ptr e(args()[0]->evaluate(variables).try_convert<entity>());
	if(e) {
		return variant(new remove_object_command(e));
	} else {
		return variant();
	}
END_FUNCTION_DEF(remove_object)

class teleport_command : public entity_command_callable
{
public:
	teleport_command(const std::string& level, const std::string& label, const std::string& transition, const entity_ptr& new_playable) 
		: level_(level), label_(label), transition_(transition), new_playable_(new_playable)
	{}

	virtual void execute(level& lvl, entity& ob) const {
		level::portal p;
		p.level_dest = level_;
		p.dest_starting_pos = true;
		p.dest_label = label_;
		p.automatic = true;
		p.transition = transition_;
		p.new_playable = new_playable_;
		lvl.force_enter_portal(p);
	}
private:
	std::string level_, label_, transition_;
	entity_ptr new_playable_;
};

FUNCTION_DEF(teleport, 1, 4, "teleport(string dest_level, (optional)string dest_label, (optional)string transition, (optional)playable): teleports the player to a new level. The level is given by dest_level, with null() for the current level. If dest_label is given then the player will be teleported to the object in the destination level with that label. If transition is given, it names are type of transition (such as 'flip' or 'fade') which indicates the kind of visual effect to use for the transition. If a playable is specified it is placed in the level instead of the current one.")
	std::string label, transition;
	entity_ptr new_playable;
	if(args().size() > 1) {
		label = args()[1]->evaluate(variables).as_string();
		if(args().size() > 2) {
			transition = args()[2]->evaluate(variables).as_string();
			if(args().size() > 3) {
				variant play = args()[3]->evaluate(variables);
				if(play.is_string()) {
					new_playable = entity_ptr(new playable_custom_object(custom_object(play.as_string(), 0, 0, 0)));
				} else {
					new_playable = play.try_convert<entity>();
				}
			}
		}
	}

	variant dst_level = args()[0]->evaluate(variables);
	const std::string dst_level_str = dst_level.is_null() ? "" : dst_level.as_string();
	return variant(new teleport_command(dst_level_str, label, transition, new_playable));
END_FUNCTION_DEF(teleport)

class schedule_command : public entity_command_callable {
public:
	schedule_command(int cycles, variant cmd) : cycles_(cycles), cmd_(cmd)
	{}

	virtual void execute(level& lvl, entity& ob) const {
		ob.add_scheduled_command(cycles_, cmd_);
	}
private:
	int cycles_;
	variant cmd_;
};

FUNCTION_DEF(schedule, 2, 2, "schedule(int cycles_in_future, list of commands): schedules the given list of commands to be run on the current object the given number of cycles in the future. Note that the object must be valid (not destroyed) and still present in the level for the commands to be run.")
	return variant(new schedule_command(
	    args()[0]->evaluate(variables).as_int(),
	    args()[1]->evaluate(variables)));
END_FUNCTION_DEF(schedule)

class add_water_command : public entity_command_callable
{
	rect r_;
	boost::array<unsigned char, 4> color_;
public:
	add_water_command(const rect& r, boost::array<unsigned char, 4> col) : r_(r), color_(col)
	{}

	virtual void execute(level& lvl, entity& ob) const {
		lvl.get_or_create_water().add_rect(r_, color_.data(), variant(&ob));
	}
};

FUNCTION_DEF(add_water, 4, 5, "add_water(int x1, int y1, int x2, int y2, (optional)[r,g,b,a]=[70,0,0,50]): adds water of the given color in the given rectangle.")
	static const unsigned char default_color[] = {70, 0, 0, 50};
	boost::array<unsigned char, 4> color;
	for(int n = 0; n != 4; ++n) {
		color[n] = default_color[n];
	}

	if(args().size() > 4) {
		variant v = args()[4]->evaluate(variables);
		ASSERT_LOG(v.is_list(), "MUST PROVIDE COLOR LIST AS FOURTH ARGUMENT TO add_water(), found: " << v.to_debug_string());
		for(int n = 0; n < 4 && n < v.num_elements(); ++n) {
			color[n] = v[n].as_int();
		}
	}

	return variant(new add_water_command(
	  rect::from_coordinates(
	    args()[0]->evaluate(variables).as_int(),
	    args()[1]->evaluate(variables).as_int(),
	    args()[2]->evaluate(variables).as_int(),
	    args()[3]->evaluate(variables).as_int()), color));
END_FUNCTION_DEF(add_water)

class remove_water_command : public entity_command_callable
{
	rect r_;
public:
	remove_water_command(const rect& r) : r_(r)
	{}

	virtual void execute(level& lvl, entity& ob) const {
		lvl.get_or_create_water().delete_rect(r_);
	}
};

FUNCTION_DEF(remove_water, 4, 4, "remove_water(int x1, int y1, int x2, int y2): removes water that has the given rectangular area.")
	return variant(new remove_water_command(
	  rect::from_coordinates(
	    args()[0]->evaluate(variables).as_int(),
	    args()[1]->evaluate(variables).as_int(),
	    args()[2]->evaluate(variables).as_int(),
	    args()[3]->evaluate(variables).as_int())));
END_FUNCTION_DEF(remove_water)

class add_wave_command : public entity_command_callable
{
	int x_, y_, xvelocity_, height_, length_, delta_height_, delta_length_;
public:
	add_wave_command(int x, int y, int xvelocity, int height, int length, int delta_height, int delta_length) : x_(x), y_(y), xvelocity_(xvelocity), height_(height), length_(length), delta_height_(delta_height), delta_length_(delta_length)
	{}

	virtual void execute(level& lvl, entity& ob) const {
		water* w = lvl.get_water();
		if(!w) {
			return;
		}

		std::cerr << "EXECUTE ADD WAVE...\n";

		point p(x_, y_);
		w->add_wave(p, xvelocity_/1000.0, height_/1000.0, length_/1000.0, delta_height_/1000.0, delta_length_/1000.0);
	}
};

FUNCTION_DEF(add_wave, 7, 7, "add_wave(int x, int y, int xvelocity, int height, int length, int delta_height, int delta_length): will add a wave with the given characteristics at the surface of the water above the (x,y) point. (x,y) must be within a body of water. Waves are a visual effect only and may not display at all on slower devices.")
	return variant(new add_wave_command(
	    args()[0]->evaluate(variables).as_int(),
	    args()[1]->evaluate(variables).as_int(),
	    args()[2]->evaluate(variables).as_int(),
	    args()[3]->evaluate(variables).as_int(),
	    args()[4]->evaluate(variables).as_int(),
	    args()[5]->evaluate(variables).as_int(),
	    args()[6]->evaluate(variables).as_int()));
END_FUNCTION_DEF(add_wave)

FUNCTION_DEF(rect_current, 7, 7, "rect_current(int x, int y, int w, int h, int xvelocity, int yvelocity, int strength) -> current generator object: creates a current generator object that has a current with the given parameters. Set the return value of this function to an object's rect_current to attach it to an object and thus place it in the level.")
	return variant(new rect_current_generator(rect(
	                  args()[0]->evaluate(variables).as_int(),
	                  args()[1]->evaluate(variables).as_int(),
	                  args()[2]->evaluate(variables).as_int(),
	                  args()[3]->evaluate(variables).as_int()),
	                  args()[4]->evaluate(variables).as_int(),
	                  args()[5]->evaluate(variables).as_int(),
	                  args()[6]->evaluate(variables).as_int()));
END_FUNCTION_DEF(rect_current)

class begin_script_command : public entity_command_callable {
public:
	explicit begin_script_command(const std::string& id) : id_(id) {
	}

	virtual void execute(level& lvl, entity& ob) const {
		lvl.begin_movement_script(id_, ob);
	}
private:
	std::string id_;
};

FUNCTION_DEF(begin_script, 1, 1, "begin_script(string id): begins the script with the given ID.")
	return variant(new begin_script_command(args()[0]->evaluate(variables).as_string()));
END_FUNCTION_DEF(begin_script)

class end_script_command : public entity_command_callable {
public:
	virtual void execute(level& lvl, entity& ob) const {
		lvl.end_movement_script();
	}
};

FUNCTION_DEF(end_script, 0, 0, "end_script(): ends the most recent script to have begun.")
	return variant(new end_script_command);
END_FUNCTION_DEF(end_script)

FUNCTION_DEF(circle_light, 2, 2, "circle_light(object, radius): creates a circle of light with the given radius")
	return variant(new circle_light(
	          *args()[0]->evaluate(variables).convert_to<custom_object>(),
			  args()[1]->evaluate(variables).as_int()));
END_FUNCTION_DEF(circle_light)

class add_particles_command : public custom_object_command_callable {
public:
	add_particles_command(const std::string& id, const std::string& type)
	  : id_(id), type_(type) {
	}

	virtual void execute(level& lvl, custom_object& ob) const {
		ob.add_particle_system(id_, type_);
	}
private:
	std::string id_, type_;
};

FUNCTION_DEF(add_particles, 1, 2, "add_particles(string id): adds the particle system with the given id to the object")
	return variant(new add_particles_command(
	    args()[0]->evaluate(variables).as_string(),
	    args()[args().size() < 2 ? 0 : 1]->evaluate(variables).as_string()));
END_FUNCTION_DEF(add_particles)

FUNCTION_DEF(collides, 4, 4, "collides(object a, string area_a, object b, string area_b) -> boolean: returns true iff area_a within object a collides with area_b within object b.")
	return variant(entity_user_collision_specific_areas(
	           *args()[0]->evaluate(variables).convert_to<entity>(),
	           args()[1]->evaluate(variables).as_string(),
	           *args()[2]->evaluate(variables).convert_to<entity>(),
	           args()[3]->evaluate(variables).as_string()));
END_FUNCTION_DEF(collides)

FUNCTION_DEF(collides_with_level, 1, 1, "collides_with_level(object) -> boolean: returns true iff the given object collides with the level.")
	return variant(non_solid_entity_collides_with_level(
	           level::current(),
	           *args()[0]->evaluate(variables).convert_to<entity>()));
END_FUNCTION_DEF(collides_with_level)

class blur_command : public custom_object_command_callable {
	int alpha_, fade_, granularity_;
public:
	blur_command(int alpha, int fade, int granularity)
	  : alpha_(alpha), fade_(fade), granularity_(granularity)
	{}

	virtual void execute(level& lvl, custom_object& ob) const {
		if(alpha_ == 0) {
			ob.set_blur(NULL);
			return;
		}

		blur_info blur(double(alpha_)/1000.0, double(fade_)/1000.0, granularity_);
		ob.set_blur(&blur);
	}
};

FUNCTION_DEF(blur, 0, 3, "blur(int alpha=0, int fade=10, int granularity=1): creates a motion blur for the current object.")
	return variant(new blur_command(
	  args().size() > 0 ? args()[0]->evaluate(variables).as_int() : 0,
	  args().size() > 1 ? args()[1]->evaluate(variables).as_int() : 10,
	  args().size() > 2 ? args()[2]->evaluate(variables).as_int() : 1));
END_FUNCTION_DEF(blur)

class text_command : public custom_object_command_callable {
public:
	text_command(const std::string& text, const std::string& font, int size, int align)
	  : text_(text), font_(font), size_(size), align_(align) {
	}

	virtual void execute(level& lvl, custom_object& ob) const {
		ob.set_text(text_, font_, size_, align_);
	}
private:
	std::string text_, font_;
	int size_;
	int align_;
};

FUNCTION_DEF(text, 1, 4, "text(string text, (optional)string font='default', (optional)int size=2, (optional)bool centered=false): adds text for the current object")
	const std::string text = args()[0]->evaluate(variables).as_string();
	const std::string font = args().size() > 1 ? args()[1]->evaluate(variables).as_string() : "default";
	const int size = args().size() > 2 ? args()[2]->evaluate(variables).as_int() : 2;

	int align = -1;
	if(args().size() > 3) {
		variant align_var = args()[3]->evaluate(variables);
		if(align_var.is_string()) {
			const std::string str = align_var.as_string();
			if(str == "left") {
				align = -1;
			} else if(str == "center") {
				align = 0;
			} else if(str == "right") {
				align = 1;
			}

		} else {
			align = align_var.as_bool() ? 0 : -1;
		}
	}
	return variant(new text_command(text, font, size, align));
END_FUNCTION_DEF(text)

class vector_text_command : public custom_object_command_callable 
{
public:
	vector_text_command(entity_ptr target, std::vector<variant>* textv)
		: target_(target)
	{
		textv_.swap(*textv);
	}

	virtual void execute(level& lvl, custom_object& ob) const 
	{
		custom_object* custom_obj = target_ ? dynamic_cast<custom_object*>(target_.get()) : &ob;
		custom_obj->clear_vector_text();
		foreach(const variant& v, textv_) {
			gui::vector_text_ptr txtp(new gui::vector_text(v));
			if(txtp) {
				custom_obj->add_vector_text(txtp);
			}
		}
	}
private:
	std::vector<variant> textv_;
	entity_ptr target_;
};

FUNCTION_DEF(textv, 1, -1, "textv(object, text_map, ...): Adds text objects to the object.  object format: {text:<string>, align: \"left|right|center\", size:<n>, rect:[x,y,w,h]}")
	entity_ptr target = args()[0]->evaluate(variables).try_convert<entity>();
	int arg_start = (target == NULL) ? 0 : 1;
	std::vector<variant> textv;
	for(int i = arg_start; i < args().size(); i++) {
		textv.push_back(args()[i]->evaluate(variables));
	}
	return variant(new vector_text_command(target, &textv));
END_FUNCTION_DEF(textv)

class clear_vector_text_command : public custom_object_command_callable 
{
public:
	clear_vector_text_command(entity_ptr target)
		: target_(target)
	{}

	virtual void execute(level& lvl, custom_object& ob) const 
	{
		custom_object* custom_obj = target_ ? dynamic_cast<custom_object*>(target_.get()) : &ob;
		custom_obj->clear_vector_text();
	}

private:
	entity_ptr target_;
};

FUNCTION_DEF(clear_textv, 0, 1, "clear_textv(object): Clears all the custom text from the object")
	entity_ptr target = args()[0]->evaluate(variables).try_convert<entity>();
	return variant(new clear_vector_text_command(target));
END_FUNCTION_DEF(clear_textv)

FUNCTION_DEF(swallow_event, 0, 0, "swallow_event(): when used in an instance-specific event handler, this causes the event to be swallowed and not passed to the object's main event handler.")
	return variant(new swallow_object_command_callable);
END_FUNCTION_DEF(swallow_event)

FUNCTION_DEF(swallow_mouse_event, 0, 0, "swallow_mouse_event(): when used in an instance-specific event handler, this causes the mouse event to be swallowed and not passed to the next object in the z-order stack.")
	return variant(new swallow_mouse_command_callable);
END_FUNCTION_DEF(swallow_mouse_event)

class set_widgets_command : public entity_command_callable {
	const entity_ptr target_;
	const std::vector<variant> widgets_;
	//const formula_callable_ptr callable_;
public:
	set_widgets_command(entity_ptr target, const std::vector<variant> widgets/*, const formula_callable_ptr callable*/)
	  : target_(target), widgets_(widgets)//, callable_(callable)
	{}

	virtual void execute(level& lvl, entity& ob) const {
		entity* e = target_ ? target_.get() : &ob;
		custom_object* custom_obj = dynamic_cast<custom_object*>(e);
		std::vector<gui::widget_ptr> w;
		foreach(const variant& v, widgets_) {
			gui::dialog_ptr dialog = boost::intrusive_ptr<gui::dialog>(v.try_convert<gui::dialog>());
			if(dialog) {
				w.push_back(dialog);
			} else {
				w.push_back(widget_factory::create(v, custom_obj));
			}
		}
		custom_obj->add_widgets(&w);
	}
};

FUNCTION_DEF(set_widgets, 1, -1, "set_widgets((optional) obj, widget, ...): Adds a group of widgets to the current object, or the specified object")
	entity_ptr target = args()[0]->evaluate(variables).try_convert<entity>();
	int arg_start = (target == NULL) ? 0 : 1;
	std::vector<variant> widgetsv;
	for(int i = arg_start; i < args().size(); i++) {
		variant items = args()[i]->evaluate(variables);
		if(items.is_list()) {
			for(int n = 0; n != items.num_elements(); ++n) {
				widgetsv.push_back(items[n]);
			}
		} else {
			widgetsv.push_back(items);
		}
	}
	//const_formula_callable_ptr callable = map_into_callable(args()[2]->evaluate(variables));
	return variant(new set_widgets_command(target, widgetsv));
END_FUNCTION_DEF(set_widgets)

class clear_widgets_command : public entity_command_callable {
	const entity_ptr target_;
public:
	clear_widgets_command(entity_ptr target) : target_(target)
	{}

	virtual void execute(level& lvl, entity& ob) const {
		entity* e = target_ ? target_.get() : &ob;
		custom_object* custom_obj = dynamic_cast<custom_object*>(e);
		custom_obj->clear_widgets();
	}
};

FUNCTION_DEF(clear_widgets, 1, 1, "clear_widgets(obj): Clears all widgets from the object.")
	entity_ptr target = args()[0]->evaluate(variables).try_convert<entity>();
	return variant(new clear_widgets_command(target));
END_FUNCTION_DEF(clear_widgets)

FUNCTION_DEF(get_widget, 2, 2, "get_widget(object obj, string id): returns the widget with the matching id for given object")
	boost::intrusive_ptr<custom_object> target = args()[0]->evaluate(variables).try_convert<custom_object>();
	std::string id = args()[1]->evaluate(variables).as_string();
	return variant(target->get_widget_by_id(id).get());
END_FUNCTION_DEF(get_widget)

class add_level_module_command : public entity_command_callable {
	std::string lvl_;
	int x_, y_;
public:
	add_level_module_command(const std::string& lvl, int x, int y)
	  : lvl_(lvl), x_(x), y_(y)
	{}

	virtual void execute(level& lvl, entity& ob) const {
		lvl.add_sub_level(lvl_, x_, y_);
	}
};

FUNCTION_DEF(add_level_module, 3, 3, "add_level_module(string lvl, int xoffset, int yoffset): adds the level module with the given level id at the given offset")
	return variant(new add_level_module_command(args()[0]->evaluate(variables).string_cast(), args()[1]->evaluate(variables).as_int(), args()[2]->evaluate(variables).as_int()));
END_FUNCTION_DEF(add_level_module)

class remove_level_module_command : public entity_command_callable {
	std::string lvl_;
public:
	explicit remove_level_module_command(const std::string& lvl) : lvl_(lvl)
	{}

	virtual void execute(level& lvl, entity& ob) const {
		lvl.remove_sub_level(lvl_);
	}
};

FUNCTION_DEF(remove_level_module, 1, 1, "remove_level_module(string lvl): removes the given level module")
	return variant(new remove_level_module_command(args()[0]->evaluate(variables).string_cast()));
END_FUNCTION_DEF(remove_level_module)

class shift_level_position_command : public entity_command_callable {
	int xoffset_, yoffset_;
public:

	shift_level_position_command(int xoffset, int yoffset)
	  : xoffset_(xoffset), yoffset_(yoffset) {}

	virtual void execute(level& lvl, entity& ob) const {
		lvl.adjust_level_offset(xoffset_, yoffset_);
	}
};

FUNCTION_DEF(cosmic_shift, 2, 2, "cosmic_shift(int xoffset, int yoffet): adjust position of all objects and tiles in the level by the given offset")
	return variant(new shift_level_position_command(args()[0]->evaluate(variables).as_int(), args()[1]->evaluate(variables).as_int()));
END_FUNCTION_DEF(cosmic_shift)

FUNCTION_DEF(rotate_rect, 4, 4, "rotate_rect(int center_x, int center_y, int rotation, int[8] rect) -> int[8]: rotates rect and returns the result")

	int center_x = args()[0]->evaluate(variables).as_int();
	int center_y = args()[1]->evaluate(variables).as_int();
	float rotate = args()[2]->evaluate(variables).as_decimal().as_float();

	variant v = args()[3]->evaluate(variables);

	ASSERT_LE(v.num_elements(), 8);
	
	GLshort r[8];
	for(int n = 0; n != v.num_elements(); ++n) {
		r[n] = v[n].as_int();
	}

	for(int n = v.num_elements(); n < 8; ++n) {
		r[n] = 0;
	}

	rotate_rect(center_x, center_y, rotate, r);

	std::vector<variant> res;
	res.reserve(8);
	for(int n = 0; n != v.num_elements(); ++n) {
		res.push_back(variant(r[n]));
	}

	return variant(&res);

END_FUNCTION_DEF(rotate_rect)

class module_pump_command : public entity_command_callable {
	module::client* client_;
public:
	explicit module_pump_command(module::client* cl) : client_(cl)
	{}

	virtual void execute(level& lvl, entity& ob) const {
		client_->process();
	}
};

class module_install_command : public entity_command_callable {
	module::client* client_;
	std::string id_;
public:
	module_install_command(module::client* cl, const std::string& id) : client_(cl), id_(id)
	{}

	virtual void execute(level& lvl, entity& ob) const {
		client_->install_module(id_);
	}
};

class module_uninstall_command : public entity_command_callable {
	std::string id_;
public:
	explicit module_uninstall_command(const std::string& id) : id_(id)
	{}

	virtual void execute(level& lvl, entity& ob) const {
		module::uninstall_downloaded_module(id_);
	}
};

FUNCTION_DEF(module_client, 0, 0, "module_client(): creates a module client object. The object will immediately start retrieving basic module info from the server. module_pump() should be called on it every frame. Has the following fields:\n  is_complete: true iff the current operation is complete and a new operation can be started. When the module_client is first created it automatically starts an operation to get the summary of modules.\n  downloaded_modules: a list of downloaded modules that are currently installed.\n  module_info: info about the modules available on the server.\n  error: contains an error string if the operation resulted in an error, null otherwise.\n  kbytes_transferred: number of kbytes transferred in the current operation\n  kbytes_total: total number of kbytes to transfer to complete the operation.")
	return variant(new module::client);
END_FUNCTION_DEF(module_client)

FUNCTION_DEF(module_pump, 1, 1, "module_pump(module_client): pumps module client events. Should be called every cycle.")
	module::client* cl = args()[0]->evaluate(variables).try_convert<module::client>();
	ASSERT_LOG(cl, "BAD ARGUMENT GIVEN TO module_pump");
	return variant(new module_pump_command(cl));
END_FUNCTION_DEF(module_pump)

FUNCTION_DEF(module_install, 2, 2, "module_install(module_client, string module_id): begins downloading the given module and installing it. This should only be called when module_client.is_complete = true (i.e. there is no operation currently underway)")
	module::client* cl = args()[0]->evaluate(variables).try_convert<module::client>();
	ASSERT_LOG(cl, "BAD ARGUMENT GIVEN TO module_pump");
	const std::string module_id = args()[1]->evaluate(variables).as_string();
	return variant(new module_install_command(cl, module_id));
END_FUNCTION_DEF(module_install)

FUNCTION_DEF(module_uninstall, 1, 1, "module_uninstall(string module_id): uninstalls the given module")
	const std::string module_id = args()[0]->evaluate(variables).as_string();
	return variant(new module_uninstall_command(module_id));
	
END_FUNCTION_DEF(module_uninstall)

class module_rate_command : public entity_command_callable {
	module::client* client_;
	std::string id_;
	int rating_;
	std::string review_;
public:
	module_rate_command(module::client* cl, const std::string& id, int rating, const std::string& review) : client_(cl), id_(id), rating_(rating), review_(review)
	{}

	virtual void execute(level& lvl, entity& ob) const {
		client_->rate_module(id_, rating_, review_);
	}
};

FUNCTION_DEF(module_rate, 3, 4, "module_rate(module_client, string module_id, int num_stars (1-5), (optional) string review): begins a request to rate the given module with the given number of stars, optionally with a review.")
	module::client* cl = args()[0]->evaluate(variables).try_convert<module::client>();
	ASSERT_LOG(cl, "BAD ARGUMENT GIVEN TO module_pump");
	const std::string module_id = args()[1]->evaluate(variables).as_string();

	const int num_stars = args()[2]->evaluate(variables).as_int();
	ASSERT_LOG(num_stars >= 1 && num_stars <= 5, "INVALID RATING: " << num_stars);
	std::string review;
	if(args().size() > 3) {
		review = args()[3]->evaluate(variables).as_string();
	}
	
	return variant(new module_rate_command(cl, module_id, num_stars, review));
END_FUNCTION_DEF(module_rate)

class module_launch_command : public entity_command_callable {
	std::string id_;
public:
	explicit module_launch_command(const std::string& id) : id_(id)
	{}

	virtual void execute(level& lvl, entity& ob) const {
		module::reload(id_);
		loadlevel::reload_level_paths();
		customobjecttype::reload_file_paths();

		const std::vector<entity_ptr> players = lvl.players();
		foreach(entity_ptr e, players) {
			lvl.remove_character(e);
		}

		level::portal p;
		p.level_dest = "titlescreen.cfg";
		p.dest_starting_pos = true;
		p.automatic = true;
		p.transition = "instant";
		p.saved_game = true; //makes it use the player in there.
		lvl.force_enter_portal(p);
	}
};

FUNCTION_DEF(module_launch, 1, 1, "module_launch(string module_id): launch the game using the given module.")
	const std::string module_id = args()[0]->evaluate(variables).as_string();
	return variant(new module_launch_command(module_id));
END_FUNCTION_DEF(module_launch)

namespace {
bool consecutive_periods(char a, char b) {
	return a == '.' && b == '.';
}
}

FUNCTION_DEF(get_document, 1, 1, "get_document(string filename): return reference to the given JSON document")
	const std::string docname = args()[0]->evaluate(variables).as_string();

	static std::map<std::string, variant> cache;
	variant& v = cache[docname];
	if(v.is_null() == false) {
		return v;
	}

	ASSERT_LOG(docname.empty() == false, "DOCUMENT NAME GIVEN TO get_document() IS EMPTY");
	ASSERT_LOG(docname[0] != '/', "DOCUMENT NAME BEGINS WITH / " << docname);
	ASSERT_LOG(std::adjacent_find(docname.begin(), docname.end(), consecutive_periods) == docname.end(), "DOCUMENT NAME CONTAINS ADJACENT PERIODS " << docname);

	try {
		const variant v = json::parse_from_file(docname);
		return v;
	} catch(json::parse_error&) {
		return variant();
	}
END_FUNCTION_DEF(get_document)

class custom_object_function_symbol_table : public function_symbol_table
{
public:
	expression_ptr create_function(
	                           const std::string& fn,
	                           const std::vector<expression_ptr>& args,
							   const formula_callable_definition* callable_def) const;
};

expression_ptr custom_object_function_symbol_table::create_function(
                           const std::string& fn,
                           const std::vector<expression_ptr>& args,
						   const formula_callable_definition* callable_def) const
{
	const std::map<std::string, function_creator*>& creators = get_function_creators(FunctionModule);
	std::map<std::string, function_creator*>::const_iterator i = creators.find(fn);
	if(i != creators.end()) {
		return expression_ptr(i->second->create(args));
	}

	return function_symbol_table::create_function(fn, args, callable_def);
}

} //namespace

bool in_speech_dialog () {return g_in_speech_dialog > 0;}

function_symbol_table& get_custom_object_functions_symbol_table()
{
	static custom_object_function_symbol_table table;
	return table;
}

void init_custom_object_functions(variant node)
{
	foreach(variant fn, node.as_list()) {
		const std::string& name = fn["name"].as_string();
		std::vector<std::string> args = util::split(fn["args"].as_string());

		const std::string* first_arg = NULL;
		const std::string* last_arg = NULL;
		if(!args.empty()) {
			first_arg = &args[0];
			last_arg = &args[0] + args.size();
		}

		game_logic::formula_callable_definition_ptr args_definition = game_logic::create_formula_callable_definition(first_arg, last_arg);

		std::vector<variant> default_args;
		recursive_function_symbol_table recursive_symbols(name, args, default_args, &get_custom_object_functions_symbol_table(), NULL);
		const_formula_ptr fml(new formula(fn["formula"], &recursive_symbols, args_definition.get()));
		get_custom_object_functions_symbol_table().add_formula_function(
		    name, fml, const_formula_ptr(), args, default_args);
		recursive_symbols.resolve_recursive_calls(fml);
		std::vector<std::string> names = get_custom_object_functions_symbol_table().get_function_names();
		assert(std::count(names.begin(), names.end(), fn["name"].as_string()));
	}
}
