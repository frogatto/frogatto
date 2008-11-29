#include <iostream>

#include "custom_object_functions.hpp"
#include "custom_object.hpp"
#include "draw_scene.hpp"
#include "entity.hpp"
#include "level.hpp"
#include "raster.hpp"
#include "texture.hpp"
#include "message_dialog.hpp"
#include "sound.hpp"
#include "string_utils.hpp"
#include "wml_node.hpp"
#include "wml_utils.hpp"


using namespace game_logic;

namespace {

class sound_command : public entity_command_callable
{
public:
	explicit sound_command(const std::string& name)
	  : name_(name)
	{}
	virtual void execute(level& lvl, entity& ob) const {
		sound::play(name_);
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
			e.reset(new custom_object(type_, x_, y_, face_right_));
		} else {
			e.reset(new character(type_, x_, y_, face_right_));
		}
		lvl.add_character(e);
		e->execute_command(instantiation_commands_);
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
						 args()[4]->evaluate(variables),
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
	explicit set_command(entity_ptr target, const std::string& attr, variant val)
	  : target_(target), attr_(attr), val_(val)
	{}
	virtual void execute(level& lvl, entity& ob) const {
		if(target_) {
			target_->mutate_value(attr_, val_);
		} else {
			ob.mutate_value(attr_, val_);
		}
	}
private:
	entity_ptr target_;
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
		entity_ptr target(args().size() == 3 ? args()[0]->evaluate(variables).convert_to<entity>() : NULL);
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

class speech_dialog_function : public function_expression {
public:
	explicit speech_dialog_function(const args_list& args)
	  : function_expression("speech_dialog", args, 3, -1) {
	}
private:
	variant execute(const formula_callable& variables) const {
		entity_ptr focus(args()[0]->evaluate(variables).try_convert<entity>());
		variant lvl_var(args()[1]->evaluate(variables));
		const level* lvl = lvl_var.try_convert<level>();
		if(focus && lvl) {
			for(int n = 0; n != 50; ++n) {
				draw_scene(*lvl, last_draw_position(), focus.get());
				SDL_GL_SwapBuffers();
				SDL_Delay(20);

				SDL_Event event;
				while(SDL_PollEvent(&event)) {
				}
			}
		}

		const std::string msg = args()[2]->evaluate(variables).as_string();
		std::vector<std::string> options;
		for(int n = 3; n < args().size(); n += 2) {
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

class debug_command : public custom_object_command_callable
{
public:
	explicit debug_command(std::string str) : str_(str)
	{}
	virtual void execute(level& lvl, custom_object& ob) const {
		std::cerr << "CUSTOM DEBUG: '" << str_ << "'\n";
	}
private:
	std::string str_;
};

class debug_function : public function_expression {
public:
	explicit debug_function(const args_list& args)
	  : function_expression("debug", args, 1) {
	}
private:
	variant execute(const formula_callable& variables) const {
		return variant(new debug_command(args()[0]->evaluate(variables).to_debug_string()));
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
	} else if(fn == "sound") {
		return expression_ptr(new sound_function(args));
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
	} else if(fn == "scroll_to") {
		return expression_ptr(new scroll_to_function(args));
	} else if(fn == "end_game") {
		return expression_ptr(new end_game_function(args));
	} else if(fn == "debug") {
		return expression_ptr(new debug_function(args));
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
