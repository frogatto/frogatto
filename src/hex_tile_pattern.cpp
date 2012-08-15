#include <string.h>

#include "asserts.hpp"
#include "foreach.hpp"
#include "formatter.hpp"
#include "hex_tile_pattern.hpp"
#include "string_utils.hpp"
#include "variant_utils.hpp"

namespace hex {

namespace {
std::deque<tile_pattern>& patterns() {
	static std::deque<tile_pattern> instance;
	return instance;
}

}

void tile_pattern::init(variant node)
{
	patterns().clear();
}

void tile_pattern::load(variant node, const std::string& tile_id)
{
	foreach(variant pattern, node["hex_tile_pattern"].as_list()) {
		patterns().push_back(tile_pattern(pattern, tile_id));
	}
}

tile_pattern::tile_pattern(variant node, const std::string& tile_id)
	: default_tile_id_(tile_id), id_(node["id"].as_string_default()), width_(-1), height_(-1), chance_(node["chance"].as_int(100))
{
	//"image"
	//"tiles"
	//"vars"
	//"pattern"
}

}
