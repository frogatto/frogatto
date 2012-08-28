#include "asserts.hpp"
#include "foreach.hpp"
#include "formatter.hpp"
#include "formula.hpp"
#include "hex_object.hpp"
#include "json_parser.hpp"
#include "module.hpp"

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

struct hex_engine 
{
	hex_engine() 
	{}

	explicit hex_engine(const variant& value)
	{
		variant functions_var = value["functions"];
		if(functions_var.is_null() == false) {
			ASSERT_LOG(functions_var.is_string() == true, "\"functions must\" be specified as a string.");
			functions.reset(new game_logic::function_symbol_table);
			game_logic::formula f(functions_var, functions.get());
		}

		variant handlers_var = value["handlers"];
		if(handlers_var.is_null() == false) {
			ASSERT_LOG(handlers_var.is_map() == true, "\"handlers\" must be specified by a map.");
			handlers.clear();
			foreach(const variant& key, handlers_var.get_keys().as_list()) {
				handlers[key.as_string()] = game_logic::formula::create_optional_formula(handlers_var[key], functions.get());
			}
		}

		variant tiles_var = value["tiles"];
		ASSERT_LOG(tiles_var.is_map(), "\"tiles\" must be a map type.");
		load_hex_tiles(tiles_var);

		rules = value["rules"].as_list_string();
	}

	boost::shared_ptr<game_logic::function_symbol_table> functions;
	std::map<std::string, game_logic::const_formula_ptr> handlers;
	std::vector<std::string> rules;
	std::map<std::string, hex_object_ptr> tiles;
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

variant hex_object::get_value(const std::string& key) const
{
	if(key == "tile") {
		return variant(tile_.get());
	} else if(key == "base_type") {
		return variant(type_);
	} else if(key == "type") {
		if(tile_) {
			return variant(tile_->key());
		}
	}
	return variant();
}

void hex_object::set_value(const std::string& key, const variant& value)
{
	if(key == "tile") {
		std::map<std::string, hex_tile_ptr>::const_iterator it = 
			get_hex_tile_map().find(value.as_string());
		ASSERT_LOG(it != get_hex_tile_map().end(), "No tile found matching type: " << value.as_string());
		tile_ = it->second;
	}
}

void hex_object::build()
{
	std::map<std::string, hex_tile_ptr>::const_iterator it = get_hex_tile_map().find(type_);
	ASSERT_LOG(it != get_hex_tile_map().end(), "\"" << type_ << "\" not found while building tiles.");
	tile_ = it->second;
}

void hex_object::apply_rules()
{
	using namespace game_logic;
	foreach(const std::string& rule, generate_hex_engine().rules) {
		//std::cerr << "Processing rule: " << rule << std::endl;
		const_formula_ptr f = generate_hex_engine().handlers[rule];
		map_formula_callable_ptr callable(new map_formula_callable());
		variant& a = callable->add_direct_access("hex");
		a = variant(this);
		variant value = f->execute(*callable.get());
		if(value.is_null() == false) {
			if(value.is_list()) {
				const int num_elements = value.num_elements();
				for(int n = 0; n != num_elements; ++n) {
					if(value[n].is_null() == false) {
						callable->execute_command(value[n]);
					}
				}
			} else {
				callable->execute_command(value);
			}
		}
	}
}

void hex_object::draw() const
{
	tile_->draw(x_, y_);
}

hex_tile::hex_tile(const std::string& key, variant node)
	: key_(key), name_(node["name"].as_string()),
	texture_(graphics::texture::get(node["image"].as_string()))
{
	foreach(const variant& tile, node["tiles"].as_list()) {
		ASSERT_LOG(tile.num_elements() == 4 && tile.is_list(), 
			"Inner elements of tiles must be a list of 4 integers.");
		rects_.push_back(rect(tile[0].as_int(), 
			tile[1].as_int(), 
			tile[2].as_int() - tile[0].as_int(), 
			tile[3].as_int() - tile[1].as_int()));
	}
}

hex_tile::~hex_tile()
{}

void hex_tile::draw(int x, int y) const
{
	const int TileSize = 72;
	const int TileSizeHalf = TileSize/2;
	const int TileSizeThreeQuarters = (TileSize*3)/4;
	const int tx = x*(abs(x)%2)*TileSizeThreeQuarters + x*(abs(x)%2==0)*TileSizeThreeQuarters;
	const int ty = TileSize*y + (abs(x)%2)*TileSizeHalf;
	graphics::blit_texture(texture_, tx, ty, rects_[0].w(), rects_[0].h(), 0.0f, 
		GLfloat(rects_[0].x())/GLfloat(texture_.width()),
		GLfloat(rects_[0].y())/GLfloat(texture_.height()),
		GLfloat(rects_[0].x2())/GLfloat(texture_.width()),
		GLfloat(rects_[0].y2())/GLfloat(texture_.height()));
}

variant hex_tile::get_value(const std::string&) const
{
	return variant();
}

void hex_tile::set_value(const std::string& key, const variant& value)
{
}

}
