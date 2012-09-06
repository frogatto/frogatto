#include "asserts.hpp"
#include "foreach.hpp"
#include "formatter.hpp"
#include "formula.hpp"
#include "hex_tile.hpp"
#include "hex_map.hpp"
#include "hex_object.hpp"
#include "json_parser.hpp"
#include "module.hpp"
#include "variant_utils.hpp"

namespace hex {

namespace {

std::map<std::string, hex_tile_ptr>& get_hex_tile_map()
{
	static std::map<std::string, hex_tile_ptr> tile_map;
	return tile_map;
}

void load_hex_tiles(variant node)
{
	if(!get_hex_tile_map().empty()) {
		get_hex_tile_map().clear();
	}
	foreach(const variant& key, node.get_keys().as_list()) {
		std::string keys = key.as_string();
		get_hex_tile_map()[keys] = hex_tile_ptr(new hex_tile(keys, node[key]));
	}
}


class find_transition_function : public game_logic::function_expression {
public:
	explicit find_transition_function(const args_list& args)
	 : function_expression("find_transition", args, 4, 4)
	{}
private:
	variant execute(const game_logic::formula_callable& variables) const {
		variant hex = args()[0]->evaluate(variables);
		hex_object_ptr hop = hex_object_ptr(hex.try_convert<hex_object>());
		ASSERT_LOG(hop != NULL, "Could not convert first parameter to valid hex_object");
		std::string this_terrain = args()[1]->evaluate(variables).as_string();
		std::vector<std::string> matching_terrain = args()[2]->evaluate(variables).as_list_string();
		std::string tt = args()[3]->evaluate(variables).as_string();

		return variant(hop->tile()->best_terrain_match(hop, this_terrain, matching_terrain, tt).get());
	}
};

class get_tile_function : public game_logic::function_expression {
public:
	explicit get_tile_function(const args_list& args)
	 : function_expression("get_tile", args, 1, 1)
	{}
private:
	variant execute(const game_logic::formula_callable& variables) const {
		std::map<std::string, hex_tile_ptr>::const_iterator it 
			= get_hex_tile_map().find(args()[0]->evaluate(variables).as_string());
		if(it == get_hex_tile_map().end()) {
			return variant();
		}
		return variant(it->second->get_single_tile().get());
	}
};

class hex_function_symbol_table : public game_logic::function_symbol_table
{
public:
	hex_function_symbol_table()
	{}

	game_logic::expression_ptr create_function(
		const std::string& fn,
		const std::vector<game_logic::expression_ptr>& args,
		const game_logic::formula_callable_definition* callable_def) const
	{
		if(fn == "get_tile") {
			return game_logic::expression_ptr(new get_tile_function(args));
		} else if(fn == "find_transition") {
			return game_logic::expression_ptr(new find_transition_function(args));
		}
		return function_symbol_table::create_function(fn, args, callable_def);
	}
};

game_logic::function_symbol_table& get_hex_function_symbol_table()
{
	static hex_function_symbol_table table;
	return table;
}

struct hex_engine 
{
	hex_engine() 
	{}

	explicit hex_engine(const variant& value)
	{
		functions_var = value["functions"];
		if(functions_var.is_null() == false) {
			ASSERT_LOG(functions_var.is_string() == true || functions_var.is_list() == true, "\"functions must\" be specified as a string or list.");
			functions.reset(new game_logic::function_symbol_table);
			functions->set_backup(&get_hex_function_symbol_table());
			if(functions_var.is_string()) {
				game_logic::formula f(functions_var, functions.get());
			} else if(functions_var.is_list()) {
				for(int n = 0; n != functions_var.num_elements(); ++n) {
					game_logic::formula f(functions_var[n], functions.get());
				}
			}
		}

		rules = value["rules"].as_list_string();

		variant tiles_var = value["tiles"];
		ASSERT_LOG(tiles_var.is_map(), "\"tiles\" must be a map type.");
		load_hex_tiles(tiles_var);

		variant handlers_var = value["handlers"];
		if(handlers_var.is_null() == false) {
			ASSERT_LOG(handlers_var.is_map() == true, "\"handlers\" must be specified by a map.");
			handlers.clear();
			foreach(const variant_pair& p, handlers_var.as_map()) {
				handlers[p.first.as_string()] = game_logic::formula::create_optional_formula(p.second, functions.get());
			}
		}
	}

	variant write() const
	{
		variant_builder res;
		res.add("functions", functions_var);
		std::map<std::string, game_logic::const_formula_ptr>::const_iterator it = handlers.begin();
		while(it != handlers.end()) {
			variant_builder node;
			node.add(it->first, it->second->str());
			res.add("handlers", node.build());
			++it;
		}
		foreach(const std::string& s, rules) {
			res.add("rules", s);
		}
		std::map<std::string, hex_tile_ptr>::const_iterator tile_it = get_hex_tile_map().begin();
		while(tile_it != get_hex_tile_map().end()) {
			variant_builder node;
			node.add(tile_it->first, tile_it->second->write());
			res.add("tiles", node.build());
			++tile_it;
		}
		return res.build();
	}

	variant functions_var;
	boost::shared_ptr<game_logic::function_symbol_table> functions;
	std::map<std::string, game_logic::const_formula_ptr> handlers;
	std::vector<std::string> rules;
};

hex_engine& generate_hex_engine()
{
	static hex_engine hexes (json::parse_from_file(module::map_file("data/hex_tiles.cfg")));
	return hexes;
}

}

hex_object::hex_object(const std::string& type, int x, int y, const hex_map* owner) 
	: owner_map_(owner), x_(x), y_(y), type_(type)
{
}

std::vector<std::string> hex_object::get_rules()
{
	return generate_hex_engine().rules;
}

hex_object_ptr hex_object::get_tile_in_dir(enum direction d) const
{
	return owner_map_->get_hex_tile(d, x_, y_);
}

hex_object_ptr hex_object::get_tile_in_dir(const std::string& s) const
{
	if(s == "north" || s == "n") {
		return get_tile_in_dir(NORTH);
	} else if(s == "south" || s == "s") {
		return get_tile_in_dir(SOUTH);
	} else if(s == "north_west" || s == "nw" || s == "northwest") {
		return get_tile_in_dir(NORTH_WEST);
	} else if(s == "north_east" || s == "ne" || s == "northeast") {
		return get_tile_in_dir(NORTH_EAST);
	} else if(s == "south_west" || s == "sw" || s == "southwest") {
		return get_tile_in_dir(SOUTH_WEST);
	} else if(s == "south_east" || s == "se" || s == "southeast") {
		return get_tile_in_dir(SOUTH_EAST);
	}
	return hex_object_ptr();
}

variant hex_object::get_value(const std::string& key) const
{
	ASSERT_LOG(owner_map_ != NULL, "Hex object not associated with a map! owner_ == NULL");
	hex_object_ptr ho = get_tile_in_dir(key);
	if(ho != NULL) {
		return variant(ho.get());
	} else if(key == "self") {
		return variant(this);
	} else if(key == "tile") {
		if(tile_) {
			return variant(tile_->owner());
		}
	} else if(key == "base_type") {
		return variant(type_);
	} else if(key == "type") {
		if(tile_) {
			return variant(tile_->type());
		}
	} else if(key == "x") {
		return variant(x_);
	} else if(key == "y") {
		return variant(y_);
	} else if(key == "xy") {
		std::vector<variant> v;
		v.push_back(variant(x_));
		v.push_back(variant(y_));
		return variant(&v);
	}
	 
	return variant();
}

void hex_object::set_value(const std::string& key, const variant& value)
{
	if(key == "tile") {
		//std::map<std::string, hex_tile_ptr>::const_iterator it 
		//	= get_hex_tile_map().find(value.as_string());
		//ASSERT_LOG(it != get_hex_tile_map().end(), "tile named \"" << value.as_string() << "\" not found");
		//if(it != get_hex_tile_map().end()) {
		//	tile_ = it->second->get_single_tile();
		//}
		tile_ = basic_hex_tile_ptr(value.try_convert<basic_hex_tile>());
		ASSERT_LOG(tile_ != NULL, "Couldn't convert tile to correct type");
	} else if(key == "transitions") {
		if(value.is_callable()) {
			transitions_.push_back(basic_hex_tile_ptr(value.try_convert<basic_hex_tile>()));
			std::cerr << "add transition1: " << transitions_.back()->type() 
				<< " : " << x_ << "," << y_ << std::endl;
		} else {
			ASSERT_LOG(value.is_list(), "transitions must be specified as a list of tiles.");
			//transitions_.clear();
			for(size_t i = 0; i < value.num_elements(); i++) {
				transitions_.push_back(basic_hex_tile_ptr(value[i].try_convert<basic_hex_tile>()));
				std::cerr << "add transition: " << transitions_.back()->type() 
					<< " : " << x_ << "," << y_ << std::endl;
			}
		}
	}
}

void hex_object::build()
{
	//std::map<std::string, hex_tile_ptr>::const_iterator it = get_hex_tile_map().find(type_);
	//ASSERT_LOG(it != get_hex_tile_map().end(), "\"" << type_ << "\" not found while building tiles.");
	//tile_ = it->second;
}

bool hex_object::execute_command(const variant& value)
{
	bool result = true;
	if(value.is_null()) {
		return result;
	}

	if(value.is_list()) {
		const int num_elements = value.num_elements();
		for(int n = 0; n != num_elements; ++n) {
			if(value[n].is_null() == false) {
				result = execute_command(value[n]) && result;
			}
		}
	} else {
		game_logic::command_callable* cmd = value.try_convert<game_logic::command_callable>();
		if(cmd != NULL) {
			cmd->execute(*this);
		}
	}
	return result;
}

void hex_object::apply_rules(const std::string& rule)
{
	using namespace game_logic;
	std::map<std::string, const_formula_ptr>::const_iterator it = generate_hex_engine().handlers.find(rule);
	ASSERT_LOG(it != generate_hex_engine().handlers.end(), "Unable to find rule \"" << rule << "\" in the list of handlers.");
	map_formula_callable_ptr callable(new map_formula_callable(this));
	variant& a = callable->add_direct_access("hex");
	a = variant(this);
	variant value = it->second->execute(*callable.get());
	execute_command(value);
}

void hex_object::draw() const
{
	// Draw base tile.
	ASSERT_LOG(tile_ != NULL, "tile is NULL");
	tile_->draw(x_, y_);
	// Draw transitions
	foreach(const const_basic_hex_tile_ptr& htp, transitions_) {
		{
			const int TileSize = 72;
			const int TileSizeHalf = TileSize/2;
			const int TileSizeThreeQuarters = (TileSize*3)/4;
			const int tx = x_*(abs(x_)%2)*TileSizeThreeQuarters + x_*(abs(x_)%2==0)*TileSizeThreeQuarters;
			const int ty = TileSize*y_ + (abs(x_)%2)*TileSizeHalf;
			graphics::draw_hollow_rect(rect(tx, ty, TileSize, TileSize).sdl_rect(), graphics::color("red").as_sdl_color());
		}
		htp->draw(x_, y_);
	}
}

}
