#include <ctype.h>

#include "asserts.hpp"
#include "controls.hpp"
#include "foreach.hpp"
#include "formula_constants.hpp"
#include "i18n.hpp"
#include "key_button.hpp"
#include "preferences.hpp"
#include "raster.hpp"
#include "string_utils.hpp"
#include "variant_utils.hpp"

namespace game_logic
{

namespace {
typedef std::map<std::string, variant> constants_map;
std::vector<constants_map> constants_stack;
}

variant get_constant(const std::string& id)
{
	if(id == "DOUBLE_SCALE") {
		return variant(preferences::double_scale());
	} else if(id == "SCREEN_WIDTH") {
		return variant(graphics::screen_width());
	} else if(id == "SCREEN_HEIGHT") {
		return variant(graphics::screen_height());
	} else if(id == "LOW_END_SYSTEM") {
#if TARGET_OS_HARMATTAN || TARGET_OS_IPHONE || TARGET_BLACKBERRY || defined(__ANDROID__)
		return variant(1);
#else
		return variant(0);
#endif
	} else if(id == "IPHONE_SYSTEM") {
#if TARGET_OS_HARMATTAN || TARGET_OS_IPHONE || TARGET_BLACKBERRY || defined(__ANDROID__)
		return variant(1);
#else
		return variant(preferences::sim_iphone() ? 1 : 0);
#endif
	} else if(id == "HIGH_END_SYSTEM") {
		return variant(!get_constant("LOW_END_SYSTEM").as_bool());
	} else if(id == "UP_KEY") {
		return variant(gui::get_key_name(controls::get_sdlkey(controls::CONTROL_UP)));
	} else if(id == "DOWN_KEY") {
		return variant(gui::get_key_name(controls::get_sdlkey(controls::CONTROL_DOWN)));
	} else if(id == "LEFT_KEY") {
		return variant(gui::get_key_name(controls::get_sdlkey(controls::CONTROL_LEFT)));
	} else if(id == "RIGHT_KEY") {
		return variant(gui::get_key_name(controls::get_sdlkey(controls::CONTROL_RIGHT)));
	} else if(id == "JUMP_KEY") {
		return variant(gui::get_key_name(controls::get_sdlkey(controls::CONTROL_JUMP)));
	} else if(id == "TONGUE_KEY") {
		return variant(gui::get_key_name(controls::get_sdlkey(controls::CONTROL_TONGUE)));
	} else if(id == "ATTACK_KEY") {
		return variant(gui::get_key_name(controls::get_sdlkey(controls::CONTROL_ATTACK)));
	} else if(id == "LOCALE") {
		return variant(i18n::get_locale());
	}

	if(constants_stack.empty() == false) {
		constants_map& m = constants_stack.back();
		constants_map::const_iterator itor = m.find(id);
		if(itor != m.end()) {
			return itor->second;
		}
	}

	return variant();
}

constants_loader::constants_loader(variant node) : same_as_base_(false)
{
	constants_map m;
	if(node.is_null() == false) {
		foreach(variant key, node.get_keys().as_list()) {
			const std::string& attr = key.as_string();
			if(std::find_if(attr.begin(), attr.end(), util::islower) != attr.end()) {
				//only all upper case are loaded as consts
				continue;
			}

			m[attr] = node[key];
		}
	}

	if(constants_stack.empty() == false && constants_stack.back() == m) {
		same_as_base_ = true;
	} else if(constants_stack.empty() == false) {
		std::cerr << "CONSTANTS ARE DIFFERENT: ";
		for(constants_map::const_iterator i = m.begin(); i != m.end(); ++i) {
			if(constants_stack.back().count(i->first) == 0) {
				std::cerr << "NOT FOUND " << i->first << " ";
			} else if(i->second != constants_stack.back()[i->first]) {
				std::cerr << "DIFF " << i->first << " ";
			}
		}

		const constants_map& m2 = constants_stack.back();
		for(constants_map::const_iterator i = m2.begin(); i != m2.end(); ++i) {
			if(m.count(i->first) == 0) {
				std::cerr << "INSERTED " << i->first << " ";
			}
		}

		std::cerr << "\n";
	}

	//std::cerr << "ADD CONSTANTS_STACK ";
	for(constants_map::const_iterator i = m.begin(); i != m.end(); ++i) {
		std::cerr << i->first << " ";
	}

	if(m.begin() != m.end()) {
		std::cerr << "\n";
	}

	constants_stack.push_back(m);
}

constants_loader::~constants_loader()
{
	ASSERT_EQ(constants_stack.empty(), false);
	constants_stack.pop_back();
	//std::cerr << "REMOVE CONSTANTS_STACK\n";
}

}
