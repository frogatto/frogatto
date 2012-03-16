#ifndef CUSTOM_OBJECT_TYPE_HPP_INCLUDED
#define CUSTOM_OBJECT_TYPE_HPP_INCLUDED

#include <map>
#include <string>

#include "boost/scoped_ptr.hpp"
#include "boost/shared_ptr.hpp"

#include "custom_object_callable.hpp"
#include "editor_variable_info.hpp"
#include "formula.hpp"
#include "formula_callable.hpp"
#include "formula_function.hpp"
#include "frame.hpp"
#include "particle_system.hpp"
#include "solid_map_fwd.hpp"
#include "variant.hpp"
#include "wml_node.hpp"

class custom_object_type;

typedef boost::shared_ptr<custom_object_type> custom_object_type_ptr;
typedef boost::shared_ptr<const custom_object_type> const_custom_object_type_ptr;

namespace wml {
class modifier;
typedef boost::shared_ptr<const modifier> const_modifier_ptr;
}

class custom_object_type
{
public:
	static wml::node_ptr merge_prototype(wml::node_ptr node);
	static const std::string* get_object_path(const std::string& id);
	static const_custom_object_type_ptr get(const std::string& id);
	static const_custom_object_type_ptr get_or_die(const std::string& id);
	static custom_object_type_ptr create(const std::string& id);
	static void invalidate_object(const std::string& id);
	static void invalidate_all_objects();
	static std::vector<const_custom_object_type_ptr> get_all();

	typedef std::vector<game_logic::const_formula_ptr> event_handler_map;

	static void init_event_handlers(wml::const_node_ptr node,
	                                event_handler_map& handlers,
									game_logic::function_symbol_table* symbols=0,
									const event_handler_map* base_handlers=NULL);

	explicit custom_object_type(wml::const_node_ptr node, const custom_object_type* base_type=NULL);
	~custom_object_type();

	const_custom_object_type_ptr get_sub_object(const std::string& id) const;

	const custom_object_callable& callable_definition() const { return callable_definition_; }

	const std::string& id() const { return id_; }
	int hitpoints() const { return hitpoints_; }

	int timer_frequency() const { return timer_frequency_; }

	const frame& default_frame() const;
	const frame& get_frame(const std::string& key) const;

	const game_logic::const_formula_ptr& next_animation_formula() const { return next_animation_formula_; }

	game_logic::const_formula_ptr get_event_handler(int event) const;
	int parallax_scale_millis_x() const {
		if(parallax_scale_millis_.get() == NULL){
			return 1000;
		}else{
			return parallax_scale_millis_->first;
		}
	}
	int parallax_scale_millis_y() const {
		if(parallax_scale_millis_.get() == NULL){
			return 1000;
		}else{
			return parallax_scale_millis_->second;
		}
	}
	
	int zorder() const { return zorder_; }
	int zsub_order() const { return zsub_order_; }
	bool is_human() const { return is_human_;}
	bool goes_inactive_only_when_standing() const { return goes_inactive_only_when_standing_; }
	bool dies_on_inactive() const { return dies_on_inactive_;}
	bool always_active() const { return always_active_;}
	bool body_harmful() const { return body_harmful_; }
	bool body_passthrough() const { return body_passthrough_; }
	bool ignore_collide() const { return ignore_collide_; }

	bool object_level_collisions() const { return object_level_collisions_; }

	int surface_friction() const { return surface_friction_; }
	int surface_traction() const { return surface_traction_; }
	int mass() const { return mass_; }

	//amount of friction we experience.
	int friction() const { return friction_; }
	int traction() const { return traction_; }
	int traction_in_air() const { return traction_in_air_; }
	int traction_in_water() const { return traction_in_water_; }

	bool respawns() const { return respawns_; }

	bool affected_by_currents() const { return affected_by_currents_; }

	wml::const_node_ptr get_child(const std::string& key) const {
		if(children_.count(key)) {
			return children_.find(key)->second;
		}

		return wml::const_node_ptr();
	}

	const_particle_system_factory_ptr get_particle_system_factory(const std::string& id) const;

	bool is_vehicle() const { return is_vehicle_; }

	int passenger_x() const { return passenger_x_; }
	int passenger_y() const { return passenger_y_; }

	int feet_width() const { return feet_width_; }

	int teleport_offset_x() const { return teleport_offset_x_; }
	int teleport_offset_y() const { return teleport_offset_y_; }
	bool no_move_to_standing() const { return no_move_to_standing_; }
	bool reverse_global_vertical_zordering() const { return reverse_global_vertical_zordering_; }

	bool serializable() const { return serializable_; }

	bool use_image_for_collisions() const { return use_image_for_collisions_; }
	bool static_object() const { return static_object_; }
	bool has_feet() const { return has_feet_; }
	bool adjust_feet_on_animation_change() const { return adjust_feet_on_animation_change_; }

	const std::map<std::string, variant>& variables() const { return variables_; }
	const std::map<std::string, variant>& tmp_variables() const { return tmp_variables_; }
	game_logic::const_map_formula_callable_ptr consts() const { return consts_; }
	const std::map<std::string, variant>& tags() const { return tags_; }

	const std::map<std::string, game_logic::const_formula_ptr>& properties() const { return properties_; }

	game_logic::function_symbol_table* function_symbols() const;

	const const_solid_info_ptr& solid() const { return solid_; }
	const const_solid_info_ptr& platform() const { return platform_; }

	const std::vector<int>& platform_offsets() const { return platform_offsets_; }

	bool solid_platform() const { return solid_platform_; }

	//true if the object can ever be solid or standable
	bool has_solid() const { return has_solid_; }

	unsigned int solid_dimensions() const { return solid_dimensions_; }
	unsigned int collide_dimensions() const { return collide_dimensions_; }

	unsigned int weak_solid_dimensions() const { return weak_solid_dimensions_; }
	unsigned int weak_collide_dimensions() const { return weak_collide_dimensions_; }

	const_custom_object_type_ptr get_variation(const std::vector<std::string>& variations) const;
	void load_variations() const;

	const_editor_entity_info_ptr editor_info() const { return editor_info_; }

	wml::const_node_ptr node() const { return node_; }

	int activation_border() const { return activation_border_; }
	const variant& available_frames() const { return available_frames_; }

	bool editor_force_standing() const { return editor_force_standing_; }

private:
	custom_object_callable callable_definition_;

	std::string id_;
	int hitpoints_;

	int timer_frequency_;

	typedef std::map<std::string, std::vector<boost::shared_ptr<frame> > > frame_map;
	frame_map frames_;
	variant available_frames_;

	boost::shared_ptr<frame> default_frame_;

	game_logic::const_formula_ptr next_animation_formula_;

	event_handler_map event_handlers_;
	boost::shared_ptr<game_logic::function_symbol_table> object_functions_;

	boost::scoped_ptr<std::pair<int, int> > parallax_scale_millis_;
	
	int zorder_;
	int zsub_order_;

	bool is_human_;
	bool goes_inactive_only_when_standing_;
	bool dies_on_inactive_;
	bool always_active_;
	bool body_harmful_;
	bool body_passthrough_;
	bool ignore_collide_;
	bool object_level_collisions_;

	int surface_friction_;
	int surface_traction_;
	int friction_, traction_, traction_in_air_, traction_in_water_;
	int mass_;

	bool respawns_;

	bool affected_by_currents_;

	std::map<std::string, wml::const_node_ptr> children_;

	wml::const_node_ptr node_;

	std::map<std::string, const_particle_system_factory_ptr> particle_factories_;

	bool is_vehicle_;
	int passenger_x_, passenger_y_;
	int feet_width_;

	bool use_image_for_collisions_, static_object_;

	bool has_feet_;

	bool adjust_feet_on_animation_change_;

	std::map<std::string, variant> variables_, tmp_variables_;
	game_logic::map_formula_callable_ptr consts_;
	std::map<std::string, variant> tags_;

	std::map<std::string, game_logic::const_formula_ptr> properties_;

	int teleport_offset_x_, teleport_offset_y_;
	bool no_move_to_standing_;
	bool reverse_global_vertical_zordering_;
	
	bool serializable_;

	const_solid_info_ptr solid_, platform_;

	bool solid_platform_;

	//variable which is true if the object is ever solid or standable
	bool has_solid_;

	unsigned int solid_dimensions_, collide_dimensions_;
	unsigned int weak_solid_dimensions_, weak_collide_dimensions_;

	int activation_border_;

	std::map<std::string, wml::const_modifier_ptr> variations_;
	mutable std::map<std::vector<std::string>, const_custom_object_type_ptr> variations_cache_;

	const_editor_entity_info_ptr editor_info_;

	std::map<std::string, const_custom_object_type_ptr> sub_objects_;

	bool editor_force_standing_;

	std::vector<int> platform_offsets_;
};

#endif
