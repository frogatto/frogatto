#include "custom_object_functions.hpp"
#include "foreach.hpp"
#include "movement_script.hpp"
#include "object_events.hpp"
#include "wml_node.hpp"
#include "wml_utils.hpp"

active_movement_script::~active_movement_script()
{
	foreach(const entity_mod& mod, mods_) {
		for(std::map<std::string, game_logic::const_formula_ptr>::const_iterator i = mod.handlers_backup.begin(); i != mod.handlers_backup.end(); ++i) {
			mod.entity->set_event_handler(get_object_event_id(i->first), i->second);
		}
		
	}
}

void active_movement_script::modify(entity_ptr entity, const std::map<std::string, game_logic::const_formula_ptr>& handlers)
{
	entity_mod mod;
	mod.entity = entity;
	for(std::map<std::string, game_logic::const_formula_ptr>::const_iterator i = handlers.begin(); i != handlers.end(); ++i) {
		mod.handlers_backup[i->first] = entity->get_event_handler(get_object_event_id(i->first));
		entity->set_event_handler(get_object_event_id(i->first), i->second);
	}

	mods_.push_back(mod);
}

movement_script::movement_script(wml::const_node_ptr node) : id_(node->attr("id")) {
	FOREACH_WML_CHILD(modification_node, node, "modification") {
		modification m;
		m.target_formula = game_logic::formula::create_optional_formula(modification_node->attr("target"), &get_custom_object_functions_symbol_table());
		for(wml::node::const_attr_iterator i = modification_node->begin_attr(); i != modification_node->end_attr(); ++i) {
			const std::string& key = i->first;
			if(key.size() > 3 && std::equal(key.begin(), key.begin() + 3, "on_")) {
				std::string event(key.begin() + 3, key.end());
				m.handlers[event] = game_logic::formula::create_optional_formula(i->second, &get_custom_object_functions_symbol_table());
			}
		}

		modifications_.push_back(m);
	}
}

active_movement_script_ptr movement_script::begin_execution(const game_logic::formula_callable& callable) const
{
	active_movement_script_ptr result(new active_movement_script);
	foreach(const modification& m, modifications_) {
		if(!m.target_formula) {
			continue;
		}

		variant v = m.target_formula->execute(callable);
		if(v.is_null()) {
			continue;
		}

		for(int n = 0; n != v.num_elements(); ++n) {
			variant obj = v[n];
			entity_ptr e(obj.try_convert<entity>());
			if(e) {
				result->modify(e, m.handlers);
			}
		}
	}

	return result;
}

wml::node_ptr movement_script::write() const
{
	wml::node_ptr result(new wml::node("script"));
	result->set_attr("id", id_);
	foreach(const modification& m, modifications_) {
		wml::node_ptr node(new wml::node("modification"));
		node->set_attr("target", m.target_formula ? m.target_formula->str() : "");
		for(std::map<std::string, game_logic::const_formula_ptr>::const_iterator i = m.handlers.begin(); i != m.handlers.end(); ++i) {
			node->set_attr("on_" + i->first, i->second ? i->second->str() : "");
		}

		result->add_child(node);
	}

	return result;
}
