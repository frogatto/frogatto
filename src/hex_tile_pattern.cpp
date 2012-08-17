#include <string.h>

#include "asserts.hpp"
#include "foreach.hpp"
#include "formatter.hpp"
#include "formula.hpp"
#include "json_parser.hpp"
#include "hex_tile_pattern.hpp"
#include "module.hpp"
#include "string_utils.hpp"
#include "variant_utils.hpp"

namespace hex {

namespace {

struct hex_engine 
{
	hex_engine() 
	{}

	explicit hex_engine(const variant& value)
	{
		variant functions_var = value["functions"];
		if(functions_var.is_string()) {
			functions.reset(new game_logic::function_symbol_table);
			game_logic::formula f(functions_var, functions.get());
		}

		variant handlers_var = value["handlers"];
		if(handlers_var.is_map()) {
			foreach(const variant& key, handlers_var.get_keys().as_list()) {
				handlers[key.as_string()] = game_logic::formula::create_optional_formula(handlers_var[key], functions.get());
			}
		}

		variant tiles_var = value["tiles"];
		ASSERT_LOG(tiles_var.is_map(), "tiles must be a map type.");
		foreach(const variant& key, tiles_var.get_keys().as_list()) {
			tiles[key.as_string()] = tiles_var[key];
		}

		rules = value["rules"].as_list_string();
	}

	boost::shared_ptr<game_logic::function_symbol_table> functions;
	std::map<std::string, game_logic::const_formula_ptr> handlers;
	std::vector<std::string> rules;
	std::map<std::string, variant> tiles;
};

hex_engine& generate_hex_engine()
{
	static hex_engine hexes (json::parse_from_file(module::map_file("hex_tiles.cfg")));
	return hexes;
}

}

void tile_pattern::load(variant node, const std::string& tile_id)
{
	using namespace game_logic;
	foreach(const std::string& rule, generate_hex_engine().rules) {
		std::cerr << "Processing rule: " << rule;
		//const_formula_ptr f = generate_hex_engine().handlers[rule];
		//variant value = f->execute();
		//map_formula_callable_ptr callable(new map_formula_callable());
		//callable->execute_command(value);
	}
}

tile_pattern::tile_pattern(variant node, const std::string& tile_id)
	: default_tile_id_(tile_id), id_(node["id"].as_string_default()), width_(-1), height_(-1), chance_(node["chance"].as_int(100))
{
}

}
