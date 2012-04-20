#ifndef NO_EDITOR
#include <map>

#include "custom_object.hpp"
#include "debug_console.hpp"
#include "editor.hpp"
#include "editor_formula_functions.hpp"
#include "filesystem.hpp"
#include "foreach.hpp"
#include "formula.hpp"
#include "formula_callable.hpp"
#include "json_parser.hpp"
#include "level.hpp"
#include "variant_utils.hpp"

namespace editor_script {

using namespace game_logic;

namespace {

const int TileSize = 32;

class editor_command : public formula_callable {
public:
	virtual ~editor_command() {}
	virtual void execute(editor& e) = 0;
private:
	variant get_value(const std::string& key) const {
		return variant();
	}
};

class add_object_command : public editor_command {
	std::string id_;
	int x_, y_;
	bool facing_;
public:
	add_object_command(const std::string& id, int x, int y, bool facing)
	  : id_(id), x_(x), y_(y), facing_(facing)
	{}
private:
	void execute(editor& e) {
		custom_object* obj = new custom_object(id_, x_, y_, facing_);
		obj->set_level(e.get_level());
		e.get_level().add_character(obj);
	}
};

class add_object_function : public function_expression {
public:
	explicit add_object_function(const args_list& args)
	  : function_expression("add_object", args, 4, 4)
	{}
private:
	variant execute(const formula_callable& variables) const {
		return variant(new add_object_command(
		        args()[0]->evaluate(variables).as_string(),
		        args()[1]->evaluate(variables).as_int(),
		        args()[2]->evaluate(variables).as_int(),
		        args()[3]->evaluate(variables).as_bool()));
	}
};

class remove_tile_rect_command : public editor_command {
	std::string tile_id_;
	int x1_, y1_, x2_, y2_;
public:
	remove_tile_rect_command(const std::string& tile_id, int x1, int y1, int x2, int y2)
	  : tile_id_(tile_id), x1_(x1), y1_(y1), x2_(x2), y2_(y2)
	{}

	void execute(editor& e) {
		e.add_tile_rect(e.get_tile_zorder(tile_id_), "", x1_, y1_, x2_, y2_);
	}
};

class remove_tiles_function : public function_expression {
public:
	explicit remove_tiles_function(const args_list& args)
	  : function_expression("remove_tiles", args, 3, 5)
	{}
private:
	variant execute(const formula_callable& variables) const {
		const std::string& tile_id = args()[0]->evaluate(variables).as_string();
		const int x1 = args()[1]->evaluate(variables).as_int();
		const int y1 = args()[2]->evaluate(variables).as_int();
		const int x2 = args().size() > 3 ? args()[3]->evaluate(variables).as_int() : x1;
		const int y2 = args().size() > 4 ? args()[4]->evaluate(variables).as_int() : y1;
		return variant(new remove_tile_rect_command(tile_id, x1*TileSize, y1*TileSize, x2*TileSize, y2*TileSize));
	}
};

class add_tile_rect_command : public editor_command {
	std::string tile_id_;
	int x1_, y1_, x2_, y2_;
public:
	add_tile_rect_command(const std::string& tile_id, int x1, int y1, int x2, int y2)
	  : tile_id_(tile_id), x1_(x1), y1_(y1), x2_(x2), y2_(y2)
	{}

	void execute(editor& e) {
		e.add_tile_rect(e.get_tile_zorder(tile_id_), tile_id_, x1_, y1_, x2_, y2_);
	}
};

class add_tiles_function : public function_expression {
public:
	explicit add_tiles_function(const args_list& args)
	  : function_expression("add_tiles", args, 3, 5)
	{}
private:
	variant execute(const formula_callable& variables) const {
		const std::string& tile_id = args()[0]->evaluate(variables).as_string();
		const int x1 = args()[1]->evaluate(variables).as_int();
		const int y1 = args()[2]->evaluate(variables).as_int();
		const int x2 = args().size() > 3 ? args()[3]->evaluate(variables).as_int() : x1;
		const int y2 = args().size() > 4 ? args()[4]->evaluate(variables).as_int() : y1;
		return variant(new add_tile_rect_command(tile_id, x1*TileSize, y1*TileSize, x2*TileSize, y2*TileSize));
	}
};

class debug_command : public editor_command
{
public:
	explicit debug_command(const std::string& str) : str_(str)
	{}
	virtual void execute(editor& e) {
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
			if(n) str += " ";
			str += args()[n]->evaluate(variables).to_debug_string();
		}

		fprintf(stderr, "DEBUG FUNCTION: %s\n", str.c_str());

		return variant(new debug_command(str));
	}
};

class editor_command_function_symbol_table : public function_symbol_table
{
public:
	static editor_command_function_symbol_table& instance() {
		static editor_command_function_symbol_table result;
		return result;
	}

	expression_ptr create_function(
	                           const std::string& fn,
	                           const std::vector<expression_ptr>& args,
							   const formula_callable_definition* callable_def) const
	{
		if(fn == "remove_tiles") {
			return expression_ptr(new remove_tiles_function(args));
		} else if(fn == "add_tiles") {
			return expression_ptr(new add_tiles_function(args));
		} else if(fn == "add_object") {
			return expression_ptr(new add_object_function(args));
		} else if(fn == "debug") {
			return expression_ptr(new debug_function(args));
		} else {
			return function_symbol_table::create_function(fn, args, callable_def);
		}
	}
};

void execute_command(variant cmd, editor& e) {
	if(cmd.is_list()) {
		for(int n = 0; n != cmd.num_elements(); ++n) {
			execute_command(cmd[n], e);
		}
	} else if(cmd.is_callable()) {
		editor_command* command = cmd.try_convert<editor_command>();
		if(command) {
			command->execute(e);
		}
	}
}

class tile_callable : public formula_callable {
public:
	tile_callable(editor& e, int x, int y)
	  : editor_(e), x_(x), y_(y)
	{}

private:
	variant get_value(const std::string& key) const {
		if(key == "x") {
			return variant(x_);
		} else if(key == "y") {
			return variant(y_);
		} else if(key == "tiles") {
			return get_tiles(x_, y_);
		} else if(key == "up") {
			return variant(new tile_callable(editor_, x_, y_-1));
		} else if(key == "down") {
			return variant(new tile_callable(editor_, x_, y_+1));
		} else if(key == "left") {
			return variant(new tile_callable(editor_, x_-1, y_));
		} else if(key == "right") {
			return variant(new tile_callable(editor_, x_+1, y_));
		} else {
			return variant();
		}
	}

	variant get_tiles(int x, int y) const {
		std::vector<variant> result;

		std::map<int, std::vector<std::string> > m;
		editor_.get_level().get_all_tiles_rect(x*TileSize, y*TileSize, x*TileSize, y*TileSize, m);
		for(std::map<int, std::vector<std::string> >::const_iterator i = m.begin(); i != m.end(); ++i) {
			foreach(const std::string& s, i->second) {
				result.push_back(variant(s));
			}
		}

		return variant(&result);
	}

	editor& editor_;
	int x_, y_;
};

class editor_command_callable : public formula_callable {
public:
	explicit editor_command_callable(editor& e) : editor_(e)
	{}
private:
	variant get_value(const std::string& key) const {
		if(key == "cells") {
			std::vector<variant> result;

			const editor::tile_selection& selection = editor_.selection();
			if(selection.empty()) {
				const rect& dim = editor_.get_level().boundaries();
				for(int y = dim.y() - dim.y()%TileSize; y < dim.y2(); y += TileSize) {
					for(int x = dim.x() - dim.x()%TileSize; x < dim.x2(); x += TileSize) {
						result.push_back(variant(new tile_callable(editor_, x/TileSize, y/TileSize)));
					}
				}
			} else {
				foreach(const point& p, selection.tiles) {
					result.push_back(variant(new tile_callable(editor_, p.x, p.y)));
				}
			}

			return variant(&result);
		} else {
			return variant();
		}
	}
	editor& editor_;
};


std::vector<info> scripts_info;
std::map<std::string, const_formula_ptr> scripts;

void load_scripts()
{
	if(scripts_info.empty() == false) {
		return;
	}

	if(!sys::file_exists("data/editor/scripts.cfg")) {
		return;
	}

	variant node = json::parse_from_file("data/editor/scripts.cfg");

	//load any functions defined here.
	foreach(variant function_node, node["function"].as_list()) {
	}

	foreach(variant script_node, node["script"].as_list()) {
		const std::string& id = script_node["id"].as_string();
		info script = { id };
		scripts_info.push_back(script);
		scripts[id].reset(new formula(script_node["script"], &editor_command_function_symbol_table::instance()));
	}
}

}

std::vector<info> all_scripts() {
	load_scripts();
	return scripts_info;
}

void execute(const std::string& id, editor& e)
{
	load_scripts();

	std::map<std::string, const_formula_ptr>::const_iterator itor = scripts.find(id);
	if(itor == scripts.end() || !itor->second) {
		return;
	}

	formula_callable_ptr callable(new editor_command_callable(e));
	const variant cmd = itor->second->execute(*callable);

	//execute the command, making sure the editor allows the user to undo the
	//entire script in one go.
	e.begin_command_group();
	execute_command(cmd, e);
	e.end_command_group();
}

}
#endif // NO_EDITOR
