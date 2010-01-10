#include <ctype.h>

#include "asserts.hpp"
#include "foreach.hpp"
#include "formula_constants.hpp"
#include "raster.hpp"
#include "wml_node.hpp"

namespace game_logic
{

namespace {
typedef std::map<std::string, variant> constants_map;
std::vector<constants_map> constants_stack;
}

variant get_constant(const std::string& id)
{
	if(id == "SCREEN_WIDTH") {
		return variant(graphics::screen_width());
	} else if(id == "SCREEN_HEIGHT") {
		return variant(graphics::screen_height());
	} else if(id == "LOW_END_SYSTEM") {
#if TARGET_OS_IPHONE
		return variant(1);
#else
		return variant(0);
#endif
	} else if(id == "IPHONE_SYSTEM") {
#if TARGET_OS_IPHONE
		return variant(1);
#else
		return variant(0);
#endif
	} else if(id == "HIGH_END_SYSTEM") {
		return variant(!get_constant("LOW_END_SYSTEM").as_bool());
	}

	foreach(const constants_map& m, constants_stack) {
		constants_map::const_iterator itor = m.find(id);
		if(itor != m.end()) {
			return itor->second;
		}
	}

	return variant();
}

constants_loader::constants_loader(wml::const_node_ptr node)
{
	constants_map m;
	if(node) {
		for(wml::node::const_attr_iterator i = node->begin_attr(); i != node->end_attr(); ++i) {
			const std::string& attr = i->first;
			if(std::find_if(attr.begin(), attr.end(), islower) != attr.end()) {
				//only all upper case are loaded as consts
				continue;
			}

			m[attr].serialize_from_string(i->second);
		}
	}

	constants_stack.push_back(m);
}

constants_loader::~constants_loader()
{
	ASSERT_EQ(constants_stack.empty(), false);
	constants_stack.pop_back();
}

}
