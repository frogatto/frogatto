#include <iostream>

#include "asserts.hpp"
#include "custom_object_functions.hpp"
#include "custom_object.hpp"
#include "debug_console.hpp"
#include "draw_scene.hpp"
#include "entity.hpp"
#include "filesystem.hpp"
#include "level.hpp"
#include "raster.hpp"
#include "texture.hpp"
#include "message_dialog.hpp"
#include "options_dialog.hpp"
#include "raster_distortion.hpp"
#include "sound.hpp"
#include "speech_dialog.hpp"
#include "string_utils.hpp"
#include "text_entry_widget.hpp"
#include "wml_parser.hpp"
#include "wml_node.hpp"
#include "wml_utils.hpp"
#include "wml_writer.hpp"

using namespace game_logic;

namespace {

class save_game_command : public entity_command_callable
	{
	public:
		virtual void execute(level& lvl, entity& ob) const {
			lvl.player()->save_game();
			sys::write_file("data/level/save.cfg", wml::output(lvl.write()));
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
		return variant(sys::file_exists("data/level/save.cfg"));
	}
};

class music_command : public entity_command_callable
	{
	public:
		explicit music_command(const std::string& name)
		: name_(name)
		{}
		virtual void execute(level& lvl, entity& ob) const {
			sound::play_music_interrupt(name_);
		}
	private:
		std::string name_;
	};

class music_function : public function_expression {
public:
	explicit music_function(const args_list& args)
	: function_expression("music",args,1,1)
	{}
private:
	variant execute(const formula_callable& variables) const {
		return variant(new music_command(
										 args()[0]->evaluate(variables).as_string()));
	}
};
	
	
class sound_command : public entity_command_callable
{
public:
	explicit sound_command(const std::string& name)
	  : name_(name)
	{}
	virtual void execute(level& lvl, entity& ob) const {
		sound::play(name_, &ob);
	}
private:
	std::string name_;
};

class sound_function : public function_expression {
public:
	explicit sound_function(const args_list& args)
	  : function_expression("sound",args,1,1)
	  {}
private:
	variant execute(const formula_callable& variables) const {
		return variant(new sound_command(
						args()[0]->evaluate(variables).as_string()));
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
	

class spawn_command : public entity_command_callable
{
public:
	spawn_command(const std::string& type, int x, int y, bool face_right, variant instantiation_commands, bool custom)
	  : type_(type), x_(x), y_(y), face_right_(face_right), instantiation_commands_(instantiation_commands), custom_(custom)
	{}
	virtual void execute(level& lvl, entity& ob) const {
		entity_ptr e;
		if(custom_) {
			e = new custom_object(type_, x_, y_, face_right_);
		} else {
			e = new character(type_, x_, y_, face_right_);
		}
		lvl.add_character(e);
		e->execute_command(instantiation_commands_);
		
		//spawn with the spawned object's midpoint (rather than its upper-left corner) at x_, y_.
		//This means objects are centered on the point they're spawned on, which is a lot more intuitive for scripting.
		e->set_pos(e->x() - e->current_frame().width() / 2 , e->y() - e->current_frame().height() / 2);
	}
private:
	std::string type_;
	int x_, y_;
	variant instantiation_commands_;
	bool face_right_;
	bool custom_;
};

class spawn_function : public function_expression {
public:
	spawn_function(const args_list& args, bool custom)
	  : function_expression("spawn", args, 4, 5), custom_(custom) {
	}

private:
	variant execute(const formula_callable& variables) const {
		return variant(new spawn_command(
		                 args()[0]->evaluate(variables).as_string(),
		                 args()[1]->evaluate(variables).as_int(),
		                 args()[2]->evaluate(variables).as_int(),
		                 args()[3]->evaluate(variables).as_int() > 0,
						 args().size() > 4 ? args()[4]->evaluate(variables) : variant(),
						 custom_));
	}

	bool custom_;
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
	explicit set_command(variant target, const std::string& attr, variant val)
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

class set_function : public function_expression {
public:
	explicit set_function(const args_list& args)
	  : function_expression("set", args, 2, 3) {
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

		custom_object* me = variables.query_value("me").convert_to<custom_object>();

		int w = args().size() >= 4 ? args()[3]->evaluate(variables).as_int() : 1;
		int h = args().size() >= 5 ? args()[4]->evaluate(variables).as_int() : 1;

		rect r(x, y, w, h);

		return variant(lvl->solid(r) || lvl->collide(r, me));
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
		if(group_ < 0 && ob.group() < 0) {
			ob.set_group(lvl.add_group());
		} else if(group_ >= 0) {
			ob.set_group(group_);
		}
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
int show_msg_dialog(const std::string& msg, const std::vector<std::string>& options)
{
	message_dialog::show_modal(msg, &options);

	int selected_option = -1;

	graphics::texture bg(graphics::texture::get_frame_buffer());

	while(message_dialog::get() != NULL) {
		graphics::blit_texture(bg, 0, 0, graphics::screen_width(), -graphics::screen_height());
		message_dialog::get()->draw();
		SDL_GL_SwapBuffers();

		selected_option = message_dialog::get()->selected_option();
		message_dialog::get()->process();

		SDL_Delay(20);
	}

	return selected_option;
}

}

class speech_dialog_command : public entity_command_callable {
public:
	explicit speech_dialog_command(const std::vector<variant>& args)
	  : args_(args)
	{}
	virtual void execute(level& lvl, entity& ob) const {

		foreach(variant var, args_) {
			if(var.is_callable()) {
				const_entity_ptr e = var.try_convert<entity>();
				if(e) {
					std::cerr << "set speaker...\n";
					dialog_.set_speaker_and_flip_side(e);
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
					SDL_Event event;
					while(SDL_PollEvent(&event)) {
						switch(event.type) {
						case SDL_KEYDOWN:
							done = dialog_.key_press(event);
							break;
						}
					}

					dialog_.process();
					draw(lvl);
				}

				if(options.empty() == false) {
					const int index = dialog_.option_selected();
					if(index >= 0 && index < option_commands.size()) {
						ob.execute_command(option_commands[index]);
					}
				}

				dialog_.set_options(std::vector<std::string>());
			}
		}
	}
private:
	void draw(const level& lvl) const {
		draw_scene(lvl, last_draw_position(), lvl.player().get());

		dialog_.draw();

		SDL_GL_SwapBuffers();
		SDL_Delay(20);
	}

	std::vector<variant> args_;

	mutable speech_dialog dialog_;
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

class dialog_function : public function_expression {
public:
	explicit dialog_function(const args_list& args)
	  : function_expression("dialog", args, 1, -1) {
	}
private:
	variant execute(const formula_callable& variables) const {
		const std::string msg = args()[0]->evaluate(variables).as_string();
		std::vector<std::string> options;
		for(int n = 1; n < args().size(); n += 2) {
			options.push_back(args()[n]->evaluate(variables).as_string());
		}

		const int selected_option = show_msg_dialog(msg, options);

		if(selected_option >= 0) {
			const int command_index = 2 + selected_option*2;
			if(command_index < args().size()) {
				return args()[command_index]->evaluate(variables);
			}
		}

		return variant();
	}
};

	
class dialog_box_command : public custom_object_command_callable
{
public:
	explicit dialog_box_command(const std::string& kind)
	: kind_(kind)
	{}
	virtual void execute(level& lvl, custom_object& ob) const {
		//lvl.set_end_game();
		if(kind_ == "options"){
			options_dialog current_dialog(100,100,200,200);
			current_dialog.show_modal();
		}
	}
private:
	std::string kind_;
};

class dialog_box_function : public function_expression {
public:
	explicit dialog_box_function(const args_list& args)
	: function_expression("dialog_box", args, 1) {
	}
private:
	variant execute(const formula_callable& variables) const {
		return variant(new dialog_box_command(
					   args()[0]->evaluate(variables).as_string()));
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
		std::cerr << "CUSTOM DEBUG: '" << str_ << "'\n";
		debug_console::add_message(str_);
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
							variant v = f.execute(callable_);
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
		draw_scene(lvl, last_draw_position(), lvl.player().get());

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

class score_command : public entity_command_callable
{
public:
	explicit score_command(int score) : score_(score)
	{}
	virtual void execute(level& lvl, entity& ob) const {
		if(lvl.player()) {
			lvl.player()->score(score_);
		}
	}
	
private:
	int score_;
};

class score_function : public function_expression {
public:
	explicit score_function(const args_list& args)
	  : function_expression("score", args, 1) {
	}
private:
	variant execute(const formula_callable& variables) const {
		return variant(new score_command(args()[0]->evaluate(variables).as_int()));
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

class teleport_command : public entity_command_callable
{
public:
	teleport_command(const std::string& level, const std::string& label) : level_(level), label_(label)
	{}

	virtual void execute(level& lvl, entity& ob) const {
		level::portal p;
		p.level_dest = level_;
		p.dest_starting_pos = true;
		p.dest_label = label_;
		p.automatic = true;
		lvl.force_enter_portal(p);
	}
private:
	std::string level_, label_;
};

class teleport_function : public function_expression {
public:
	explicit teleport_function(const args_list& args)
	  : function_expression("teleport", args, 1, 2) {
	}
private:
	variant execute(const formula_callable& variables) const {
		std::string label;
		if(args().size() > 1) {
			label = args()[1]->evaluate(variables).as_string();
		}
		return variant(new teleport_command(args()[0]->evaluate(variables).as_string(), label));
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

class custom_object_function_symbol_table : public function_symbol_table
{
public:
	expression_ptr create_function(
	                           const std::string& fn,
	                           const std::vector<expression_ptr>& args) const;
};

expression_ptr custom_object_function_symbol_table::create_function(
                           const std::string& fn,
                           const std::vector<expression_ptr>& args) const
{
	if(fn == "spawn") {
		return expression_ptr(new spawn_function(args, true));
	} else if(fn == "char") {
		return expression_ptr(new spawn_function(args, false));
	} else if(fn == "save_game") {
		return expression_ptr(new save_game_function(args));
	} else if(fn == "load_game") {
		return expression_ptr(new load_game_function(args));
	} else if(fn == "can_load_game") {
		return expression_ptr(new can_load_game_function(args));
	} else if(fn == "sound") {
		return expression_ptr(new sound_function(args));
	} else if(fn == "music") {
		return expression_ptr(new music_function(args));
	} else if(fn == "stop_sound") {
		return expression_ptr(new stop_sound_function(args));
	} else if(fn == "shake_screen") {
		return expression_ptr(new shake_screen_function(args));
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
		return expression_ptr(new set_function(args));
	} else if(fn == "powerup") {
		return expression_ptr(new get_powerup_function(args));
	} else if(fn == "solid") {
		return expression_ptr(new solid_function(args));
	} else if(fn == "dialog") {
		return expression_ptr(new dialog_function(args));
	} else if(fn == "speech_dialog") {
		return expression_ptr(new speech_dialog_function(args));
	} else if(fn == "dialog_box") {
		return expression_ptr(new dialog_box_function(args));
	} else if(fn == "scroll_to") {
		return expression_ptr(new scroll_to_function(args));
	} else if(fn == "end_game") {
		return expression_ptr(new end_game_function(args));
	} else if(fn == "debug") {
		return expression_ptr(new debug_function(args));
	} else if(fn == "debug_console") {
		return expression_ptr(new debug_console_function(args));
	} else if(fn == "score") {
		return expression_ptr(new score_function(args));
	} else if(fn == "distortion") {
		return expression_ptr(new distortion_function(args));
	} else if(fn == "get_object") {
		return expression_ptr(new get_object_function(args));
	} else if(fn == "teleport") {
		return expression_ptr(new teleport_function(args));
	} else if(fn == "add_wave") {
		return expression_ptr(new add_wave_function(args));
	}
	return function_symbol_table::create_function(fn, args);
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
		get_custom_object_functions_symbol_table().add_formula_function(
		    i1->second->attr("name"),
			const_formula_ptr(new formula(i1->second->attr("formula"),
			                    &get_custom_object_functions_symbol_table())),
			const_formula_ptr(),
		    util::split(i1->second->attr("args")));
		std::vector<std::string> names = get_custom_object_functions_symbol_table().get_function_names();
		assert(std::count(names.begin(), names.end(), i1->second->attr("name")));
	}
}
