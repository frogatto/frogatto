#include "custom_object_functions.hpp"
#include "foreach.hpp"
#include "movement_script.hpp"
#include "object_events.hpp"
#include "variant_utils.hpp"

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

movement_script::movement_script(variant node) : id_(node["id"].as_string()) {
	foreach(variant modification_node, node["modification"].as_list()) {
		modification m;
		m.target_formula = game_logic::formula::create_optional_formula(modification_node["target"], &get_custom_object_functions_symbol_table());
		foreach(const variant_pair& value, modification_node.as_map()) {
			const std::string& key = value.first.as_string();
			if(key.size() > 3 && std::equal(key.begin(), key.begin() + 3, "on_")) {
				std::string event(key.begin() + 3, key.end());
				m.handlers[event] = game_logic::formula::create_optional_formula(value.second, &get_custom_object_functions_symbol_table());
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

variant movement_script::write() const
{
	variant_builder result;
	result.add("id", id_);
	foreach(const modification& m, modifications_) {
		variant_builder node;
		node.add("target", m.target_formula ? m.target_formula->str() : "");
		for(std::map<std::string, game_logic::const_formula_ptr>::const_iterator i = m.handlers.begin(); i != m.handlers.end(); ++i) {
			node.add("on_" + i->first, i->second ? i->second->str() : "");
		}

		result.add("modification", node.build());
	}

	return result.build();
}
