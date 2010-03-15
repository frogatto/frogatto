#include <boost/bind.hpp>
#include <iostream>

#include "asserts.hpp"
#include "blur.hpp"
#include "collision_utils.hpp"
#include "controls.hpp"
#include "current_generator.hpp"
#include "custom_object_functions.hpp"
#include "custom_object.hpp"
#include "debug_console.hpp"
#include "draw_scene.hpp"
#include "entity.hpp"
#include "filesystem.hpp"
#include "formula_callable_definition.hpp"
#include "level.hpp"
#include "level_runner.hpp"
#include "object_events.hpp"
#include "player_info.hpp"
#include "powerup.hpp"
#include "raster.hpp"
#include "texture.hpp"
#include "message_dialog.hpp"
#include "options_dialog.hpp"
#include "playable_custom_object.hpp"
#include "random.hpp"
#include "raster_distortion.hpp"
#include "sound.hpp"
#include "speech_dialog.hpp"
#include "string_utils.hpp"
#include "text_entry_widget.hpp"
#include "thread.hpp"
#include "wml_parser.hpp"
#include "wml_node.hpp"
#include "wml_utils.hpp"
#include "wml_writer.hpp"
#include "preferences.hpp"

using namespace game_logic;

namespace {

class save_game_command : public entity_command_callable
	{
	public:
		virtual void execute(level& lvl, entity& ob) const {
			lvl.player()->get_entity().save_game();
			sys::write_file(preferences::save_file_path(), wml::output(lvl.write()));
		}
	};

class save_game_function : public function_expression {
public:
	explicit save_game_function(const args_list& args)
	: function_expression("save_game",args,0)
	{}
private:
	variant execute(const formula_callable& variables) const {
		return variant(new save_game_command());
	}
};

class load_game_command : public entity_command_callable
	{
	public:
		virtual void execute(level& lvl, entity& ob) const {
			level::portal p;
			p.level_dest = "save.cfg";
			p.dest_starting_pos = true;
			p.saved_game = true;
			lvl.force_enter_portal(p);
		}
	};

class load_game_function : public function_expression {
public:
	explicit load_game_function(const args_list& args)
	: function_expression("load_game",args,0)
	{}
private:
	variant execute(const formula_callable& variables) const {
		return variant(new load_game_command());
	}
};

class can_load_game_function : public function_expression {
public:
	explicit can_load_game_function(const args_list& args)
	: function_expression("can_load_game",args,0)
	{}
private:
	variant execute(const formula_callable& variables) const {
		return variant(sys::file_exists(preferences::save_file_path()));
	}
};

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

class music_function : public function_expression {
public:
	explicit music_function(const args_list& args)
	: function_expression("music",args,1,2)
	{}
private:
	variant execute(const formula_callable& variables) const {
		return variant(new music_command(
										 args()[0]->evaluate(variables).as_string(),
										 true));
	}
};

	//play music just once, rather than permanently changing the song
class music_onetime_function : public function_expression {
public:
	explicit music_onetime_function(const args_list& args)
	: function_expression("music_onetime",args,1,2)
	{}
private:
	variant execute(const formula_callable& variables) const {
		return variant(new music_command(
										 args()[0]->evaluate(variables).as_string(),
										 false));
	}
};
	
class sound_command : public entity_command_callable
{
public:
	explicit sound_command(const std::string& name, const bool loops)
	  : name_(name), loops_(loops)
	{}
	virtual void execute(level& lvl, entity& ob) const {
		if(loops_){
			sound::play_looped(name_, &ob);
		}else{
			std::cerr << "PLAY SOUND COMMAND: " << name_ << "\n";
			sound::play(name_, &ob);
		}
	}
private:
	std::string name_;
	bool loops_;
};

class sound_function : public function_expression {
public:
	explicit sound_function(const args_list& args)
	  : function_expression("sound",args,1,2)
	  {}
private:
	variant execute(const formula_callable& variables) const {
		return variant(new sound_command(
										 args()[0]->evaluate(variables).as_string(),
										 false));
	}
};

	
class sound_loop_function : public function_expression {
public:
	explicit sound_loop_function(const args_list& args)
	: function_expression("sound_loop",args,1,2)
	{}
private:
	variant execute(const formula_callable& variables) const {
		return variant(new sound_command(
										 args()[0]->evaluate(variables).as_string(),
										 true));
	}
};

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
	
	
	
class sound_volume_function : public function_expression {
public:
	explicit sound_volume_function(const args_list& args)
	: function_expression("sound_volume",args,1,1)
	{}
private:
	variant execute(const formula_callable& variables) const {
		return variant(new sound_volume_command(
										 args()[0]->evaluate(variables).as_int()));
	}
};
	
	
class unboard_command : public entity_command_callable {
public:
	virtual void execute(level& lvl, entity& ob) const {
		std::cerr << "UNBOARD\n";
		ob.unboarded(lvl);
	}
};

class unboard_function : public function_expression {
public:
	explicit unboard_function(const args_list& args)
	  : function_expression("unboard",args,0,0)
	{}
private:
	variant execute(const formula_callable& variables) const {
		return variant(new unboard_command);
	}
};

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

class stop_sound_function : public function_expression {
public:
	explicit stop_sound_function(const args_list& args)
	  : function_expression("stop_sound",args,1,1)
	  {}
private:
	variant execute(const formula_callable& variables) const {
		return variant(new stop_sound_command(
						args()[0]->evaluate(variables).as_string()));
	}
};

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

class title_function : public function_expression {
public:
	explicit title_function(const args_list& args)
	  : function_expression("title",args,1,2)
	  {}
private:
	variant execute(const formula_callable& variables) const {
		return variant(new title_command(
		  args()[0]->evaluate(variables).as_string(),
		  args().size() >= 2 ? args()[1]->evaluate(variables).as_int() : 50));
	}
};

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

class shake_screen_function : public function_expression {
public:
	explicit shake_screen_function(const args_list& args)
	: function_expression("shake_screen",args,4,4)
	{}
private:
	variant execute(const formula_callable& variables) const {
		return variant(new shake_screen_command(
										args()[0]->evaluate(variables).as_int(),
										args()[1]->evaluate(variables).as_int(),
										args()[2]->evaluate(variables).as_int(),
										args()[3]->evaluate(variables).as_int() ));
	}
};

class radial_current_function : public function_expression {
public:
	explicit radial_current_function(const args_list& args)
	  : function_expression("radial_current", args, 2, 2)
	{}
private:
	variant execute(const formula_callable& variables) const {
		return variant(new radial_current_generator(args()[0]->evaluate(variables).as_int(), args()[1]->evaluate(variables).as_int()));
	}
};

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

class execute_function : public function_expression
{
public:
	explicit execute_function(const args_list& args)
	  : function_expression("execute", args, 2, 2) {
	}

private:
	variant execute(const formula_callable& variables) const {
		entity_ptr e(args()[0]->evaluate(variables).convert_to<entity>());
		variant cmd = args()[1]->evaluate(variables);
		return variant(new execute_on_command(e, cmd));
	}
};

class spawn_command : public entity_command_callable
{
public:
	spawn_command(const std::string& type, int x, int y, bool face_right, variant instantiation_commands, bool player)
	  : type_(type), x_(x), y_(y), face_right_(face_right), instantiation_commands_(instantiation_commands), player_(player)
	{}
	virtual void execute(level& lvl, entity& ob) const {
		custom_object* obj = new custom_object(type_, x_, y_, face_right_);
		
		if(player_) {
			//make a playable version of this object
			custom_object* player = new playable_custom_object(*obj);
			delete obj;
			obj = player;
		}

		obj->set_level(lvl);
		entity_ptr e = obj;
		lvl.add_character(e);
		
		//spawn with the spawned object's midpoint (rather than its upper-left corner) at x_, y_.
		//This means objects are centered on the point they're spawned on, which is a lot more intuitive for scripting.
		e->set_pos(e->x() - e->current_frame().width() / 2 , e->y() - e->current_frame().height() / 2);

		e->execute_command(instantiation_commands_);

		//send an event to the parent to let them know they've spawned a child,
		//and let them record the child's details.
		game_logic::map_formula_callable* spawn_callable(new game_logic::map_formula_callable);
		variant holder(spawn_callable);
		spawn_callable->add("parent", variant(&ob));
		spawn_callable->add("child", variant(obj));
		ob.handle_event("child_spawned", spawn_callable);
		obj->handle_event("spawned", spawn_callable);

		if(entity_collides(level::current(), *e, MOVE_NONE)) {
			lvl.remove_character(e);
		}
	}
private:
	std::string type_;
	int x_, y_;
	variant instantiation_commands_;
	bool face_right_;
	bool player_;
};

class spawn_function : public function_expression {
public:
	spawn_function(const args_list& args, bool player)
	  : function_expression("spawn", args, 4, 5), player_(player) {
	}

private:
	variant execute(const formula_callable& variables) const {
		return variant(new spawn_command(
		                 args()[0]->evaluate(variables).as_string(),
		                 args()[1]->evaluate(variables).as_int(),
		                 args()[2]->evaluate(variables).as_int(),
		                 args()[3]->evaluate(variables).as_int() > 0,
						 args().size() > 4 ? args()[4]->evaluate(variables) : variant(),
						 player_));
	}

	bool player_;
};

class object_function : public function_expression {
public:
	explicit object_function(const args_list& args)
	  : function_expression("object", args, 4, 5) {
	}

private:
	variant execute(const formula_callable& variables) const {
		//generate a random number just so we mark this as being a
		//function which shouldn't have its result cached.
		rng::generate();

		const std::string type = args()[0]->evaluate(variables).as_string();
		const int x = args()[1]->evaluate(variables).as_int();
		const int y = args()[2]->evaluate(variables).as_int();
		const bool face_right = args()[3]->evaluate(variables).as_int() > 0;
		custom_object* obj = new custom_object(type, x, y, face_right);

		if(args().size() > 4) {
			variant properties = args()[4]->evaluate(variables);
			variant keys = properties.get_keys();
			for(int n = 0; n != keys.num_elements(); ++n) {
				variant value = properties[keys[n]];
				obj->mutate_value(keys.as_string(), value);
			}
		}

		return variant(obj);
	}
};

class board_vehicle_command : public entity_command_callable {
	entity_ptr vehicle_;
public:
	explicit board_vehicle_command(entity_ptr vehicle) : vehicle_(vehicle)
	{}

	virtual void execute(level& lvl, entity& ob) const {
		vehicle_->boarded(lvl, entity_ptr(&ob));
	}
};

class board_vehicle_function : public function_expression {
public:
	explicit board_vehicle_function(const args_list& args)
	  : function_expression("board_vehicle", args, 1, 1)
	{}
private:
	variant execute(const formula_callable& variables) const {
		entity_ptr vehicle = args()[0]->evaluate(variables).try_convert<entity>();
		if(vehicle) {
			return variant(new board_vehicle_command(vehicle));
		} else {
			return variant();
		}
	}
};

class eject_vehicle_command : public entity_command_callable {
public:
	virtual void execute(level& lvl, entity& ob) const {
		ob.unboarded(lvl);
	}
};

class eject_vehicle_function : public function_expression {
public:
	explicit eject_vehicle_function(const args_list& args)
	  : function_expression("eject_vehicle", args, 0, 0)
	{}
private:
	variant execute(const formula_callable& variables) const {
		return variant(new eject_vehicle_command());
	}
};

class child_command : public custom_object_command_callable
{
public:
	child_command(const std::string& type, int x, int y)
	  : type_(type), x_(x), y_(y)
	{}
	virtual void execute(level& lvl, custom_object& ob) const {
		wml::const_node_ptr node = ob.get_child(type_);
		if(node) {
			entity_ptr e(entity::build(node));
			if(x_ || y_) {
				e->set_pos(x_, y_);
			}
			lvl.add_character(e);
		}
	}
private:
	std::string type_;
	int x_, y_;
};

class child_function : public function_expression {
public:
	explicit child_function(const args_list& args)
	  : function_expression("child", args, 1, 3) {
	}

private:
	variant execute(const formula_callable& variables) const {
		return variant(new child_command(
		                  args()[0]->evaluate(variables).as_string(),
		  args().size() >= 2 ? args()[1]->evaluate(variables).as_int() : 0,
		  args().size() >= 3 ? args()[2]->evaluate(variables).as_int() : 0));
	}
};

class hit_command : public entity_command_callable
{
public:
	explicit hit_command(entity_ptr e) : e_(e)
	{}

private:
	void execute(level& lvl, entity& ob) const {
		e_->hit_by(ob);
	}
	entity_ptr e_;
};

class hit_function : public function_expression {
public:
	explicit hit_function(const args_list& args)
	  : function_expression("hit", args, 1, 1) {
	}

private:
	variant execute(const formula_callable& variables) const {
		std::cerr << "hit_function\n";
		variant var(args()[0]->evaluate(variables));
		entity_ptr e(var.try_convert<entity>());
		if(e.get()) {
			return variant(new hit_command(e));
		} else {
			std::cerr << "ERROR: hit function given bad argument\n";
			return variant();
		}
	}
};

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

class animation_function : public function_expression {
public:
	explicit animation_function(const args_list& args)
	  : function_expression("animation", args, 1) {
	}

private:
	variant execute(const formula_callable& variables) const {
		return variant(new animation_command(args()[0]->evaluate(variables).as_string()));
	}
};

class die_command : public custom_object_command_callable
{
public:
	virtual void execute(level& lvl, custom_object& ob) const {
		ob.die();
	}
}; 

class die_function : public function_expression {
public:
	explicit die_function(const args_list& args)
	  : function_expression("die", args, 0) {
	}
private:
	variant execute(const formula_callable& variables) const {
		return variant(new die_command());
	}
};

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

class facing_function : public function_expression {
public:
	explicit facing_function(const args_list& args)
	  : function_expression("facing", args, 1) {
	}
private:
	variant execute(const formula_callable& variables) const {
		return variant(new facing_command(args()[0]->evaluate(variables).as_int()));
	}
};

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

class set_var_function : public function_expression {
public:
	explicit set_var_function(const args_list& args)
	  : function_expression("set_var", args, 2) {
	}
private:
	variant execute(const formula_callable& variables) const {
		return variant(new set_var_command(
		    args()[0]->evaluate(variables).as_string(),
			args()[1]->evaluate(variables)));
	}
};

class set_command : public entity_command_callable
{
public:
	set_command(variant target, const std::string& attr, variant val)
	  : target_(target), attr_(attr), val_(val)
	{}
	virtual void execute(level& lvl, entity& ob) const {
		if(target_.is_callable()) {
			target_.mutable_callable()->mutate_value(attr_, val_);
		} else {
			ob.mutate_value(attr_, val_);
		}
	}
private:
	variant target_;
	std::string attr_;
	variant val_;
};

class set_by_slot_command : public entity_command_callable
{
public:
	set_by_slot_command(int slot, const variant& value)
	  : slot_(slot), value_(value)
	{}

	virtual void execute(level& lvl, entity& obj) const {
		obj.mutate_value_by_slot(slot_, value_);
	}

	void set_value(const variant& value) { value_ = value; }

private:
	int slot_;
	variant value_;
};

class set_function : public function_expression {
public:
	set_function(const args_list& args, const formula_callable_definition* callable_def)
	  : function_expression("set", args, 2, 3), slot_(-1) {
		if(args.size() == 2) {
			variant literal = args[0]->is_literal();
			if(literal.is_string()) {
				key_ = literal.as_string();
			} else {
				args[0]->is_identifier(&key_);
			}

			if(!key_.empty() && callable_def) {
				slot_ = callable_def->get_slot(key_);
				if(slot_ != -1) {
					cmd_ = boost::intrusive_ptr<set_by_slot_command>(new set_by_slot_command(slot_, variant()));
				}
			}
		}
	}
private:
	variant execute(const formula_callable& variables) const {
		if(slot_ != -1) {
			if(cmd_->refcount() == 1) {
				cmd_->set_value(args()[1]->evaluate(variables));
				return variant(cmd_.get());
			}

			cmd_ = boost::intrusive_ptr<set_by_slot_command>(new set_by_slot_command(slot_, args()[1]->evaluate(variables)));
			return variant(cmd_.get());
		}

		if(!key_.empty()) {
			return variant(new set_command(variant(), key_, args()[1]->evaluate(variables)));
		}

		if(args().size() == 2) {
			try {
				std::string member;
				variant target = args()[0]->evaluate_with_member(variables, member);
				return variant(new set_command(
				  target, member, args()[1]->evaluate(variables)));
			} catch(game_logic::formula_error&) {
			}
		}

		variant target;
		if(args().size() == 3) {
			target = args()[0]->evaluate(variables);
		}
		const int begin_index = args().size() == 2 ? 0 : 1;
		return variant(new set_command(
		    target,
		    args()[begin_index]->evaluate(variables).as_string(),
			args()[begin_index + 1]->evaluate(variables)));
	}

	std::string key_;
	int slot_;
	mutable boost::intrusive_ptr<set_by_slot_command> cmd_;
};

class set_value_function : public function_expression {
public:
	explicit set_value_function(const args_list& args)
	  : function_expression("set_value", args, 2, 3) {
	}
private:
	variant execute(const formula_callable& variables) const {
		variant target;
		if(args().size() == 3) {
			target = args()[0]->evaluate(variables);
		}
		const int begin_index = args().size() == 2 ? 0 : 1;
		return variant(new set_command(
		    target,
		    args()[begin_index]->evaluate(variables).as_string(),
			args()[begin_index + 1]->evaluate(variables)));
	}
};

class get_powerup_command : public entity_command_callable
{
public:
	explicit get_powerup_command(entity_ptr target, const std::string& powerup_id)
	  : target_(target), powerup_id_(powerup_id)
	{}

	virtual void execute(level& lvl, entity& ob) const {
		target_->get_powerup(powerup_id_);
	}

private:
	entity_ptr target_;
	std::string powerup_id_;
};

class get_powerup_function : public function_expression {
public:
	explicit get_powerup_function(const args_list& args)
	  : function_expression("set", args, 2, 2) {
	}
private:
	variant execute(const formula_callable& variables) const {
		std::cerr << "POWERUP FUNCTION\n";
		entity_ptr target(args()[0]->evaluate(variables).convert_to<entity>());
		const std::string id = args()[1]->evaluate(variables).as_string();
		return variant(new get_powerup_command(target, id));
	}
};

class solid_function : public function_expression {
public:
	explicit solid_function(const args_list& args)
	  : function_expression("solid", args, 3, 5) {
	}
private:
	variant execute(const formula_callable& variables) const {
		level* lvl = args()[0]->evaluate(variables).convert_to<level>();
		const int x = args()[1]->evaluate(variables).as_int();
		const int y = args()[2]->evaluate(variables).as_int();

		int w = args().size() >= 4 ? args()[3]->evaluate(variables).as_int() : 1;
		int h = args().size() >= 5 ? args()[4]->evaluate(variables).as_int() : 1;

		rect r(x, y, w, h);

		return variant(lvl->solid(r));
	}
};

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

class set_solid_function : public function_expression {
public:
	explicit set_solid_function(const args_list& args)
	  : function_expression("set_solid", args, 4, 5) {
	}
private:
	variant execute(const formula_callable& variables) const {
		return variant(new set_solid_command(
		  rect::from_coordinates(
			args()[0]->evaluate(variables).as_int(),
			args()[1]->evaluate(variables).as_int(),
			args()[2]->evaluate(variables).as_int(),
			args()[3]->evaluate(variables).as_int()),
			args().size() > 4 ? args()[4]->evaluate(variables).as_bool() : false));
	}
};

class group_size_function : public function_expression {
public:
	explicit group_size_function(const args_list& args)
	  : function_expression("group_size", args, 2) {
	}
private:
	variant execute(const formula_callable& variables) const {
		level* lvl = args()[0]->evaluate(variables).convert_to<level>();
		return variant(lvl->group_size(args()[1]->evaluate(variables).as_int()));
	}
};

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

class set_group_function : public function_expression {
public:
	explicit set_group_function(const args_list& args)
	  : function_expression("set_group", args, 0, 1) {
	}
private:
	variant execute(const formula_callable& variables) const {
		int group = -1;
		if(args().size() > 0) {
			group = args()[0]->evaluate(variables).as_int();
		}

		return variant(new set_group_command(group));
	}
};

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
			SDL_Delay(20);
		}
	}
private:
	entity_ptr focus_;
};

class scroll_to_function : public function_expression {
public:
	explicit scroll_to_function(const args_list& args)
	  : function_expression("scroll_to", args, 1, 1) {
	}
private:
	variant execute(const formula_callable& variables) const {
		return variant(new scroll_to_command(args()[0]->evaluate(variables).try_convert<entity>()));
	}
};

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

class transient_speech_dialog_function : public function_expression {
public:
	explicit transient_speech_dialog_function(const args_list& args)
	  : function_expression("transient_speech_dialog", args, 1, -1) {
	}
private:
	variant execute(const formula_callable& variables) const {
		entity_ptr speaker;
		int duration = 50;

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

		return variant(&result);
	}
};

class speech_dialog_command : public custom_object_command_callable {
public:
	explicit speech_dialog_command(const std::vector<variant>& args, bool paused=false)
	  : args_(args), paused_(paused)
	{}
	virtual void execute(level& lvl, custom_object& ob) const {
//		pause_scope pauser;

		if(g_in_speech_dialog) {
			return;
		}

		//make it so the player's controls become locked for the duration of the dialog.
		controls::local_controls_lock controller_lock;

		in_speech_dialog_tracker dialog_tracker;

		foreach(variant var, args_) {
			if(var.is_callable()) {
				const_entity_ptr e = var.try_convert<entity>();
				if(e) {
					std::cerr << "set speaker...\n";
					dialog_.set_speaker_and_flip_side(e);
				}

				const entity_command_callable* entity_cmd = var.try_convert<const entity_command_callable>();
				if(entity_cmd) {
					entity_cmd->execute(lvl, ob);
				}

				const custom_object_command_callable* obj_cmd = var.try_convert<const custom_object_command_callable>();
				if(obj_cmd) {
					obj_cmd->execute(lvl, ob);
				}
			}

			if(var.is_list()) {
				std::vector<variant> option_commands;
				std::vector<std::string> options;
				std::vector<std::string> message;
				for(int n = 0; n != var.num_elements(); ++n) {
					if(message.empty() == false && var[n].is_list()) {
						options.push_back(message.back());
						message.pop_back();
						option_commands.push_back(var[n]);
					} else {
						message.push_back(var[n].as_string());
					}
				}

				dialog_.set_text(message);
				dialog_.set_options(options);

				bool done = false;
				while(!done) {
					if(!paused_) {
						lvl.process();
						lvl.process_draw();
					}

					SDL_Event event;
					while(SDL_PollEvent(&event)) {
						switch(event.type) {
						case SDL_MOUSEBUTTONDOWN:
						case SDL_KEYDOWN:
							done = done || dialog_.key_press(event);
							break;
						}
					}

					done = done || dialog_.process();
					draw(lvl);
				}

				if(options.empty() == false) {
					const int index = dialog_.option_selected();
					if(index >= 0 && index < option_commands.size()) {
						dialog_tracker.cancel();
						ob.execute_command(option_commands[index]);
					}
				}

				dialog_.set_options(std::vector<std::string>());
			}
		}
	}
private:
	void draw(const level& lvl) const {
		draw_scene(lvl, last_draw_position(), &lvl.player()->get_entity());

		dialog_.draw();

		SDL_GL_SwapBuffers();
		SDL_Delay(20);
	}

	std::vector<variant> args_;

	mutable speech_dialog dialog_;
	bool paused_;
};

class speech_dialog_function : public function_expression {
public:
	explicit speech_dialog_function(const args_list& args)
	  : function_expression("speech_dialog", args, 1, -1) {
	}
private:
	variant execute(const formula_callable& variables) const {
		std::vector<variant> v;
		for(int n = 0; n != args().size(); ++n) {
			v.push_back(args()[n]->evaluate(variables));
		}

		return variant(new speech_dialog_command(v));
	}
};

class paused_speech_dialog_function : public function_expression {
public:
	explicit paused_speech_dialog_function(const args_list& args)
	  : function_expression("paused_speech_dialog", args, 1, -1) {
	}
private:
	variant execute(const formula_callable& variables) const {
		std::vector<variant> v;
		for(int n = 0; n != args().size(); ++n) {
			v.push_back(args()[n]->evaluate(variables));
		}

		return variant(new speech_dialog_command(v, true));
	}
};

class end_game_command : public custom_object_command_callable
{
public:
	virtual void execute(level& lvl, custom_object& ob) const {
		lvl.set_end_game();
	}
};

class end_game_function : public function_expression {
public:
	explicit end_game_function(const args_list& args)
	  : function_expression("end_game", args, 0, 0) {
	}
private:
	variant execute(const formula_callable& variables) const {
		return variant(new end_game_command());
	}
};

class debug_command : public entity_command_callable
{
public:
	explicit debug_command(const std::string& str) : str_(str)
	{}
	virtual void execute(level& lvl, entity& ob) const {
		debug_console::add_message(str_);
		std::cerr << "CONSOLE: " << str_ << "\n";
	}
private:
	std::string str_;
};

class debug_function : public function_expression {
public:
	explicit debug_function(const args_list& args)
	  : function_expression("debug", args, 1, -1) {
	}
private:
	variant execute(const formula_callable& variables) const {
		std::string str;
		for(int n = 0; n != args().size(); ++n) {
			str += args()[n]->evaluate(variables).to_debug_string();
		}

		fprintf(stderr, "DEBUG FUNCTION: %s\n", str.c_str());

		return variant(new debug_command(str));
	}
};

#if !TARGET_IPHONE_SIMULATOR && !TARGET_OS_IPHONE
class debug_console_command : public entity_command_callable
{
public:
	explicit debug_console_command(const formula_callable& callable)
	  : callable_(callable), history_pos_(0) {
		entry_.set_loc(10, 300);
		entry_.set_dim(300, 20);
	}

	virtual void execute(level& lvl, entity& ob) const {
		history_.clear();
		history_pos_ = 0;

		bool done = false;
		while(!done) {
			SDL_Event event;
			while(SDL_PollEvent(&event)) {
				switch(event.type) {
				case SDL_KEYDOWN:
					done = event.key.keysym.sym == SDLK_ESCAPE;

					if(event.key.keysym.sym == SDLK_RETURN) {
						try {
							const std::string text = entry_.text();
							history_.push_back(entry_.text());
							history_pos_ = history_.size();
							entry_.set_text("");
							game_logic::formula f(text, &get_custom_object_functions_symbol_table());
							variant v = f.execute(ob);
							ob.execute_command(v);
							debug_console::add_message(v.to_debug_string());
						} catch(game_logic::formula_error&) {
							debug_console::add_message("error parsing formula");
						} catch(...) {
							debug_console::add_message("unknown error parsing formula");
						}
					} else if(event.key.keysym.sym == SDLK_UP) {
						if(history_pos_ > 0) {
							--history_pos_;
							ASSERT_LT(history_pos_, history_.size());
							entry_.set_text(history_[history_pos_]);
						}
					} else if(event.key.keysym.sym == SDLK_DOWN) {
						if(!history_.empty() && history_pos_ < history_.size() - 1) {
							++history_pos_;
							entry_.set_text(history_[history_pos_]);
						} else {
							history_pos_ = history_.size();
							entry_.set_text("");
						}
					}
					break;
				}

				entry_.process_event(event, false);
			}

			draw(lvl);
			SDL_Delay(20);
		}
	}
private:
	void draw(const level& lvl) const {
		draw_scene(lvl, last_draw_position(), &lvl.player()->get_entity());

		entry_.draw();

		SDL_GL_SwapBuffers();
	}

	const formula_callable& callable_;
	mutable gui::text_entry_widget entry_;
	mutable std::vector<std::string> history_;
	mutable int history_pos_;
};

class debug_console_function : public function_expression {
public:
	explicit debug_console_function(const args_list& args)
	  : function_expression("debug_console", args, 0, 0) {
	}
private:
	variant execute(const formula_callable& variables) const {
		return variant(new debug_console_command(variables));
	}
};
#endif

class fire_event_command : public entity_command_callable {
	const entity_ptr target_;
	const std::string event_;
	const const_formula_callable_ptr callable_;
public:
	fire_event_command(entity_ptr target, const std::string& event, const_formula_callable_ptr callable)
	  : target_(target), event_(event), callable_(callable)
	{}

	virtual void execute(level& lvl, entity& ob) const {
		entity* e = target_ ? target_.get() : &ob;
		e->handle_event(event_, callable_.get());
	}
};

class fire_event_function : public function_expression {
public:
	explicit fire_event_function(const args_list& args)
	  : function_expression("fire_event", args, 1, 3) {
	}
private:
	variant execute(const formula_callable& variables) const {
		entity_ptr target;
		std::string event;
		const_formula_callable_ptr callable;

		if(args().size() == 3) {
			target = args()[0]->evaluate(variables).convert_to<entity>();
			event = args()[1]->evaluate(variables).as_string();
			callable = args()[2]->evaluate(variables).as_callable();
		} else if(args().size() == 2) {
			variant v1 = args()[0]->evaluate(variables);
			variant v2 = args()[1]->evaluate(variables);
			if(v1.is_string()) {
				event = v1.as_string();
				callable = v2.as_callable();
			} else {
				target = v1.convert_to<entity>();
				event = v2.as_string();
			}
		} else {
			event = args()[0]->evaluate(variables).as_string();
		}

		return variant(new fire_event_command(target, event, callable));
	}
};

class distortion_function : public function_expression {
public:
	explicit distortion_function(const args_list& args)
	  : function_expression("distortion", args, 3) {
	}
private:
	variant execute(const formula_callable& variables) const {
		return variant(new graphics::radial_distortion(
		                       args()[0]->evaluate(variables).as_int(),
		                       args()[1]->evaluate(variables).as_int(),
		                       args()[2]->evaluate(variables).as_int()));
	}
};

class get_object_function : public function_expression {
public:
	explicit get_object_function(const args_list& args)
	  : function_expression("get_object", args, 2) {
	}
private:
	variant execute(const formula_callable& variables) const {
		level* lvl = args()[0]->evaluate(variables).try_convert<level>();
		if(lvl) {
			return variant(lvl->get_entity_by_label(args()[1]->evaluate(variables).as_string()).get());
		}

		return variant();
	}
};

//a command which moves an object in a given direction enough to resolve
//any solid conflicts.
class resolve_solid_command : public entity_command_callable {
	entity_ptr e_;
	int xdir_, ydir_, max_cycles_;
public:
	resolve_solid_command(entity_ptr e, int xdir, int ydir, int max_cycles) : e_(e), xdir_(xdir), ydir_(ydir), max_cycles_(max_cycles)
	{}

	virtual void execute(level& lvl, entity& ob) const {
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

class resolve_solid_function : public function_expression {
public:
	explicit resolve_solid_function(const args_list& args)
	  : function_expression("resolve_solid", args, 3, 4) {
	}
private:
	variant execute(const formula_callable& variables) const {
		entity_ptr e(args()[0]->evaluate(variables).try_convert<entity>());
		const int xdir = args()[1]->evaluate(variables).as_int();
		const int ydir = args()[2]->evaluate(variables).as_int();
		const int max_cycles = args().size() > 3 ? args()[3]->evaluate(variables).as_int() : 100;
		if(e) {
			return variant(new resolve_solid_command(e, xdir, ydir, max_cycles));
		} else {
			return variant();
		}
	}
};

class add_object_command : public entity_command_callable {
	entity_ptr e_;
public:
	explicit add_object_command(entity_ptr e) : e_(e)
	{}

	virtual void execute(level& lvl, entity& ob) const {
		if(place_entity_in_level(lvl, *e_)) {
			lvl.add_character(e_);
		}
	}
};

class add_object_function : public function_expression {
public:
	explicit add_object_function(const args_list& args)
	  : function_expression("add_object", args, 1) {
	}
private:
	variant execute(const formula_callable& variables) const {
		entity_ptr e(args()[0]->evaluate(variables).try_convert<entity>());
		if(e) {
			return variant(new add_object_command(e));
		} else {
			std::cerr << "OBJECT NOT VALID!\n";
			return variant();
		}
	}
};

class remove_object_command : public entity_command_callable {
	entity_ptr e_;
public:
	explicit remove_object_command(entity_ptr e) : e_(e)
	{}

	virtual void execute(level& lvl, entity& ob) const {
		lvl.remove_character(e_);
	}
};

class remove_object_function : public function_expression {
public:
	explicit remove_object_function(const args_list& args)
	  : function_expression("remove_object", args, 1) {
	}
private:
	variant execute(const formula_callable& variables) const {
		entity_ptr e(args()[0]->evaluate(variables).try_convert<entity>());
		if(e) {
			return variant(new remove_object_command(e));
		} else {
			return variant();
		}
	}
};

class teleport_command : public entity_command_callable
{
public:
	teleport_command(const std::string& level, const std::string& label, const std::string& transition) : level_(level), label_(label), transition_(transition)
	{}

	virtual void execute(level& lvl, entity& ob) const {
		level::portal p;
		p.level_dest = level_;
		p.dest_starting_pos = true;
		p.dest_label = label_;
		p.automatic = true;
		p.transition = transition_;
		lvl.force_enter_portal(p);
	}
private:
	std::string level_, label_, transition_;
};

class teleport_function : public function_expression {
public:
	explicit teleport_function(const args_list& args)
	  : function_expression("teleport", args, 1, 3) {
	}
private:
	variant execute(const formula_callable& variables) const {
		std::string label, transition;
		if(args().size() > 1) {
			label = args()[1]->evaluate(variables).as_string();
			if(args().size() > 2) {
				transition = args()[2]->evaluate(variables).as_string();
			}
		}
		return variant(new teleport_command(args()[0]->evaluate(variables).as_string(), label, transition));
	}
};

class schedule_command : public entity_command_callable {
public:
	schedule_command(int cycles, variant cmd) : cycles_(cycles), cmd_(cmd)
	{}

	virtual void execute(level& lvl, entity& ob) const {
		ob.add_scheduled_command(lvl.cycle() + cycles_, cmd_);
	}
private:
	int cycles_;
	variant cmd_;
};

class schedule_function : public function_expression {
public:
	explicit schedule_function(const args_list& args)
	  : function_expression("schedule", args, 2, 2) {
	}
private:
	variant execute(const formula_callable& variables) const {
		return variant(new schedule_command(
		    args()[0]->evaluate(variables).as_int(),
		    args()[1]->evaluate(variables)));
	}
};

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

class add_water_function : public function_expression {
public:
	explicit add_water_function(const args_list& args)
	  : function_expression("add_water", args, 4, 5) {
	}
private:
	variant execute(const formula_callable& variables) const {
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
	}
};

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

class add_wave_function : public function_expression {
public:
	explicit add_wave_function(const args_list& args)
	  : function_expression("add_wave", args, 7) {
	}
private:
	variant execute(const formula_callable& variables) const {
		return variant(new add_wave_command(
		    args()[0]->evaluate(variables).as_int(),
		    args()[1]->evaluate(variables).as_int(),
		    args()[2]->evaluate(variables).as_int(),
		    args()[3]->evaluate(variables).as_int(),
		    args()[4]->evaluate(variables).as_int(),
		    args()[5]->evaluate(variables).as_int(),
		    args()[6]->evaluate(variables).as_int()));
	}
};

class rect_current_function : public function_expression {
public:
	explicit rect_current_function(const args_list& args)
           : function_expression("rect_current", args, 7) {
	}
private:
	variant execute(const formula_callable& variables) const {
		return variant(new rect_current_generator(rect(
		                  args()[0]->evaluate(variables).as_int(),
		                  args()[1]->evaluate(variables).as_int(),
		                  args()[2]->evaluate(variables).as_int(),
		                  args()[3]->evaluate(variables).as_int()),
		                  args()[4]->evaluate(variables).as_int(),
		                  args()[5]->evaluate(variables).as_int(),
		                  args()[6]->evaluate(variables).as_int()));
	}
};

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

class begin_script_function : public function_expression {
public:
	explicit begin_script_function(const args_list& args)
	   : function_expression("begin_script", args, 1) {
	}
private:
	variant execute(const formula_callable& variables) const {
		return variant(new begin_script_command(args()[0]->evaluate(variables).as_string()));
	}
};

class end_script_command : public entity_command_callable {
public:
	virtual void execute(level& lvl, entity& ob) const {
		lvl.end_movement_script();
	}
};

class end_script_function : public function_expression {
public:
	explicit end_script_function(const args_list& args)
	   : function_expression("end_script", args, 0) {
	}

	variant execute(const formula_callable& variables) const {
		return variant(new end_script_command);
	}
};

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

class add_particles_function : public function_expression {
public:
	explicit add_particles_function(const args_list& args)
	  : function_expression("add_particles", args, 1, 2) {
	}

	variant execute(const formula_callable& variables) const {
		return variant(new add_particles_command(
		    args()[0]->evaluate(variables).as_string(),
		    args()[args().size() < 2 ? 0 : 1]->evaluate(variables).as_string()));
	}
};

class collides_function : public function_expression {
public:
	explicit collides_function(const args_list& args)
	  : function_expression("collides", args, 4, 4) {
	}

	variant execute(const formula_callable& variables) const {
		return variant(entity_user_collision_specific_areas(
		           *args()[0]->evaluate(variables).convert_to<entity>(),
		           args()[1]->evaluate(variables).as_string(),
		           *args()[2]->evaluate(variables).convert_to<entity>(),
		           args()[3]->evaluate(variables).as_string()));
	}
};

class blur_command : public custom_object_command_callable {
	int alpha_, fade_, granularity_;
public:
	blur_command(int alpha, int fade, int granularity)
	  : alpha_(alpha), fade_(fade), granularity_(granularity)
	{}

	virtual void execute(level& lvl, custom_object& ob) const {
		blur_info blur(double(alpha_)/1000.0, double(fade_)/1000.0, granularity_);
		ob.set_blur(&blur);
	}
};

class blur_function : public function_expression {
public:
	explicit blur_function(const args_list& args)
	  : function_expression("blur", args, 0, 3) {
	}
	
	variant execute(const formula_callable& variables) const {
		return variant(new blur_command(
		  args().size() > 0 ? args()[0]->evaluate(variables).as_int() : 0,
		  args().size() > 1 ? args()[1]->evaluate(variables).as_int() : 10,
		  args().size() > 2 ? args()[2]->evaluate(variables).as_int() : 1));
	}
};

class text_command : public custom_object_command_callable {
public:
	text_command(const std::string& text, const std::string& font)
	  : text_(text), font_(font) {
	}

	virtual void execute(level& lvl, custom_object& ob) const {
		ob.set_text(text_, font_);
	}
private:
	std::string text_, font_;
};

class text_function : public function_expression {
public:
	explicit text_function(const args_list& args)
	  : function_expression("text", args, 1, 2) {
	}
	
	variant execute(const formula_callable& variables) const {
		const std::string text = args()[0]->evaluate(variables).as_string();
		const std::string font = args().size() > 1 ? args()[1]->evaluate(variables).as_string() : "default";
		return variant(new text_command(text, font));
	}
};

class preload_powerup_command : public custom_object_command_callable {
public:
	explicit preload_powerup_command(const std::string& id)
	  : id_(id)
	{}

	virtual void execute(level& lvl, custom_object& ob) const {
		std::cerr << "PRELOAD_POWERUP: '" << id_ << "'\n";

		if(!lvl.player()){
			return;
		}

		entity_ptr ch = &lvl.player()->get_entity();

		if(ch->driver()) {
			ch = ch->driver();
		}

		const_powerup_ptr p = powerup::get(id_);
		if(!p) {
			return;
		}

		if(ch->is_powerup_loaded(p)) {
			//this modification is already cached, so don't bother.
			return;
		}

		ch->preload_powerup(p);

		//threading::thread thread(boost::bind(do_preload_powerup, p, lvl.player()));
		//thread.detach();
	}
private:
	std::string id_;
};

class preload_powerup_function : public function_expression {
public:
	explicit preload_powerup_function(const args_list& args)
	  : function_expression("preload_powerup", args, 1, 1) {
	}

	variant execute(const formula_callable& variables) const {
		return variant(new preload_powerup_command(args()[0]->evaluate(variables).as_string()));
	}
};

class swallow_event_function : public function_expression {
public:
	explicit swallow_event_function(const args_list& args)
	  : function_expression("swallow_event", args, 0, 0) {
	}

	variant execute(const formula_callable& variables) const {
		return variant(new swallow_object_command_callable);
	}
};

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
	if(fn == "execute") {
		return expression_ptr(new execute_function(args));
	} else if(fn == "spawn") {
		return expression_ptr(new spawn_function(args, false));
	} else if(fn == "spawn_player") {
		return expression_ptr(new spawn_function(args, true));
	} else if(fn == "object") {
		return expression_ptr(new object_function(args));
	} else if(fn == "board_vehicle") {
		return expression_ptr(new board_vehicle_function(args));
	} else if(fn == "eject_vehicle") {
		return expression_ptr(new eject_vehicle_function(args));
	} else if(fn == "save_game") {
		return expression_ptr(new save_game_function(args));
	} else if(fn == "load_game") {
		return expression_ptr(new load_game_function(args));
	} else if(fn == "can_load_game") {
		return expression_ptr(new can_load_game_function(args));
	} else if(fn == "sound") {
		return expression_ptr(new sound_function(args));
	} else if(fn == "sound_loop") {
		return expression_ptr(new sound_loop_function(args));
	} else if(fn == "sound_volume") {
		return expression_ptr(new sound_volume_function(args));
	} else if(fn == "music") {
		return expression_ptr(new music_function(args));
	} else if(fn == "music_onetime") {
		return expression_ptr(new music_onetime_function(args));
	} else if(fn == "unboard") {
		return expression_ptr(new unboard_function(args));
	} else if(fn == "stop_sound") {
		return expression_ptr(new stop_sound_function(args));
	} else if(fn == "title") {
		return expression_ptr(new title_function(args));
	} else if(fn == "shake_screen") {
		return expression_ptr(new shake_screen_function(args));
	} else if(fn == "radial_current") {
		return expression_ptr(new radial_current_function(args));
	} else if(fn == "child") {
		return expression_ptr(new child_function(args));
	} else if(fn == "hit") {
		return expression_ptr(new hit_function(args));
	} else if(fn == "animation") {
		return expression_ptr(new animation_function(args));
	} else if(fn == "die") {
		return expression_ptr(new die_function(args));
	} else if(fn == "facing") {
		return expression_ptr(new facing_function(args));
	} else if(fn == "set_var") {
		return expression_ptr(new set_var_function(args));
	} else if(fn == "set") {
		return expression_ptr(new set_function(args, callable_def));
	} else if(fn == "set_value") {
		return expression_ptr(new set_value_function(args));
	} else if(fn == "powerup") {
		return expression_ptr(new get_powerup_function(args));
	} else if(fn == "solid") {
		return expression_ptr(new solid_function(args));
	} else if(fn == "set_solid") {
		return expression_ptr(new set_solid_function(args));
	} else if(fn == "speech_dialog") {
		return expression_ptr(new speech_dialog_function(args));
	} else if(fn == "paused_speech_dialog") {
		return expression_ptr(new paused_speech_dialog_function(args));
	} else if(fn == "transient_speech_dialog") {
		return expression_ptr(new transient_speech_dialog_function(args));
	} else if(fn == "set_group") {
		return expression_ptr(new set_group_function(args));
	} else if(fn == "scroll_to") {
		return expression_ptr(new scroll_to_function(args));
	} else if(fn == "end_game") {
		return expression_ptr(new end_game_function(args));
	} else if(fn == "debug") {
		return expression_ptr(new debug_function(args));
#if !TARGET_IPHONE_SIMULATOR && !TARGET_OS_IPHONE
	} else if(fn == "debug_console") {
		return expression_ptr(new debug_console_function(args));
#endif
	} else if(fn == "fire_event") {
		return expression_ptr(new fire_event_function(args));
	} else if(fn == "distortion") {
		return expression_ptr(new distortion_function(args));
	} else if(fn == "get_object") {
		return expression_ptr(new get_object_function(args));
	} else if(fn == "add_object") {
		return expression_ptr(new add_object_function(args));
	} else if(fn == "resolve_solid") {
		return expression_ptr(new resolve_solid_function(args));
	} else if(fn == "remove_object") {
		return expression_ptr(new remove_object_function(args));
	} else if(fn == "teleport") {
		return expression_ptr(new teleport_function(args));
	} else if(fn == "schedule") {
		return expression_ptr(new schedule_function(args));
	} else if(fn == "add_water") {
		return expression_ptr(new add_water_function(args));
	} else if(fn == "add_wave") {
		return expression_ptr(new add_wave_function(args));
	} else if(fn == "rect_current") {
		return expression_ptr(new rect_current_function(args));
	} else if(fn == "begin_script") {
		return expression_ptr(new begin_script_function(args));
	} else if(fn == "end_script") {
		return expression_ptr(new end_script_function(args));
	} else if(fn == "add_particles") {
		return expression_ptr(new add_particles_function(args));
	} else if(fn == "collides") {
		return expression_ptr(new collides_function(args));
	} else if(fn == "blur") {
		return expression_ptr(new blur_function(args));
	} else if(fn == "text") {
		return expression_ptr(new text_function(args));
	} else if(fn == "preload_powerup") {
		return expression_ptr(new preload_powerup_function(args));
	} else if(fn == "swallow_event") {
		return expression_ptr(new swallow_event_function(args));
	}

	return function_symbol_table::create_function(fn, args, callable_def);
}

} //namespace

function_symbol_table& get_custom_object_functions_symbol_table()
{
	static custom_object_function_symbol_table table;
	return table;
}

void init_custom_object_functions(wml::const_node_ptr node)
{
	wml::node::const_child_iterator i1 = node->begin_child("function");
	wml::node::const_child_iterator i2 = node->end_child("function");
	for(; i1 != i2; ++i1) {
		const std::string& name = i1->second->attr("name");
		std::vector<std::string> args = util::split(i1->second->attr("args"));

		game_logic::formula_callable_definition_ptr args_definition = game_logic::create_formula_callable_definition(&args[0], &args[0] + args.size());

		recursive_function_symbol_table recursive_symbols(name, args, &get_custom_object_functions_symbol_table());
		const_formula_ptr fml(new formula(i1->second->attr("formula"), &recursive_symbols, args_definition.get()));
		get_custom_object_functions_symbol_table().add_formula_function(
		    name, fml, const_formula_ptr(), args);
		recursive_symbols.resolve_recursive_calls(fml);
		std::vector<std::string> names = get_custom_object_functions_symbol_table().get_function_names();
		assert(std::count(names.begin(), names.end(), i1->second->attr("name").val()));
	}
}
