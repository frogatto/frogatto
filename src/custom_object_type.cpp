#include <cassert>
#include <iostream>

#include "asserts.hpp"
#include "custom_object_functions.hpp"
#include "custom_object_type.hpp"
#include "filesystem.hpp"
#include "wml_node.hpp"
#include "wml_parser.hpp"
#include "wml_utils.hpp"

namespace {
std::map<std::string, std::string> object_file_paths;

typedef std::map<std::string, const_custom_object_type_ptr> object_map;
object_map cache;
}

const_custom_object_type_ptr custom_object_type::get(const std::string& id)
{
	object_map::const_iterator itor = cache.find(id);
	if(itor != cache.end()) {
		return itor->second;
	}

	if(object_file_paths.empty()) {
		//find out the paths to all our files
		sys::get_unique_filenames_under_dir("data/objects", &object_file_paths);
	}

	//find the file for the object we are loading.
	std::map<std::string, std::string>::const_iterator path_itor = object_file_paths.find(id + ".cfg");
	ASSERT_LOG(path_itor != object_file_paths.end(), "Could not find file for object '" << id << "'");

	try {
		//create the object and add it to our cache.
		custom_object_type_ptr result(new custom_object_type(wml::parse_wml(sys::read_file(path_itor->second))));
		cache[id] = result;
		return result;
	} catch(wml::parse_error& e) {
		ASSERT_LOG(false, "Error parsing WML for custom object in " << path_itor->second << ": " << e.message);
	} catch(...) {
		ASSERT_LOG(false, "Unknown error loading custom object in " << path_itor->second);
	}
}

void custom_object_type::init_event_handlers(wml::const_node_ptr node,
                                             event_handler_map& handlers)
{
	for(wml::node::const_attr_iterator i = node->begin_attr(); i != node->end_attr(); ++i) {
		if(i->first.size() > 3 && std::equal(i->first.begin(), i->first.begin() + 3, "on_")) {
			const std::string event(i->first.begin() + 3, i->first.end());
			handlers[event] = game_logic::formula::create_optional_formula(i->second, &get_custom_object_functions_symbol_table());
		}
	}
}

custom_object_type::custom_object_type(wml::const_node_ptr node)
  : id_(node->attr("id")),
	hitpoints_(wml::get_int(node, "hitpoints", 1)),
	timer_frequency_(wml::get_int(node, "timer_frequency", -1)),
	zorder_(wml::get_int(node, "zorder")),
	dies_on_inactive_(wml::get_bool(node, "dies_on_inactive", false)),
	always_active_(wml::get_bool(node, "always_active", false)),
    body_harmful_(wml::get_bool(node, "body_harmful", true)),
    body_passthrough_(wml::get_bool(node, "body_passthrough", false)),
    ignore_collide_(wml::get_bool(node, "ignore_collide", false)),
    springiness_(wml::get_int(node, "springiness")),
	surface_friction_(wml::get_int(node, "surface_friction", 20)),
	surface_traction_(wml::get_int(node, "surface_traction", 100)),
	friction_(wml::get_int(node, "friction")),
	on_players_side_(wml::get_bool(node, "on_players_side", false)),
	respawns_(wml::get_bool(node, "respawns", true)),
	affected_by_currents_(wml::get_bool(node, "affected_by_currents", false))
{
	wml::node::const_child_iterator a1 = node->begin_child("animation");
	wml::node::const_child_iterator a2 = node->end_child("animation");
	for(; a1 != a2; ++a1) {
		boost::shared_ptr<frame> f(new frame(a1->second));
		frames_[a1->second->attr("id")].push_back(f);
		const int duplicates = atoi(a1->second->attr("duplicates").c_str());
		if(duplicates > 1) {
			for(int n = 1; n != duplicates; ++n) {
				frames_[a1->second->attr("id")].push_back(f);
			}
		}
		if(!default_frame_) {
			default_frame_ = f;
		}
	}

	mass_ = wml::get_int(node, "mass", (default_frame_->collide_w() * default_frame_->collide_h() ) );
	
	wml::node::const_child_iterator c1 = node->begin_child("child");
	wml::node::const_child_iterator c2 = node->end_child("child");
	for(; c1 != c2; ++c1) {
		const std::string& child_id = c1->second->attr("child_id");
		children_[child_id] = c1->second;
	}

	assert(default_frame_);

	next_animation_formula_ = game_logic::formula::create_optional_formula(node->attr("next_animation"), &get_custom_object_functions_symbol_table());

	init_event_handlers(node, event_handlers_);

}

const frame& custom_object_type::default_frame() const
{
	return *default_frame_;
}

const frame& custom_object_type::get_frame(const std::string& key) const
{
	frame_map::const_iterator itor = frames_.find(key);
	if(itor == frames_.end() || itor->second.empty()) {
		return default_frame();
	} else {
//		if(itor->second.size() == 1) {
			return *itor->second.front().get();
//		} else {
//			return *itor->second[rand()%itor->second.size()].get();
//		}
	}
}

game_logic::const_formula_ptr custom_object_type::get_event_handler(const std::string& event) const
{
	std::map<std::string, game_logic::const_formula_ptr>::const_iterator i = event_handlers_.find(event);
	if(i != event_handlers_.end()) {
		return i->second;
	} else {
		return game_logic::const_formula_ptr();
	}
}
