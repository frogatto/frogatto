#ifndef CUSTOM_OBJECT_TYPE_HPP_INCLUDED
#define CUSTOM_OBJECT_TYPE_HPP_INCLUDED

#include <map>
#include <string>

#include "boost/scoped_ptr.hpp"
#include "boost/shared_ptr.hpp"

#include "formula.hpp"
#include "frame.hpp"
#include "wml_node.hpp"

class custom_object_type;

typedef boost::shared_ptr<custom_object_type> custom_object_type_ptr;
typedef boost::shared_ptr<const custom_object_type> const_custom_object_type_ptr;

class custom_object_type
{
public:
	static void init(wml::const_node_ptr node);
	static const_custom_object_type_ptr get(const std::string& id);

	typedef std::map<std::string, game_logic::const_formula_ptr> event_handler_map;

	static void init_event_handlers(wml::const_node_ptr node,
	                                event_handler_map& handlers);

	explicit custom_object_type(wml::const_node_ptr node);

	const std::string& id() const { return id_; }
	int hitpoints() const { return hitpoints_; }

	const frame& default_frame() const;
	const frame& get_frame(const std::string& key) const;

	const game_logic::const_formula_ptr& next_animation_formula() const { return next_animation_formula_; }

	game_logic::const_formula_ptr get_event_handler(const std::string& event) const;

	int zorder() const { return zorder_; }
	bool body_harmful() const { return body_harmful_; }
	bool body_passthrough() const { return body_passthrough_; }
	bool ignore_collide() const { return ignore_collide_; }

	int springiness() const { return springiness_; }
	int surface_friction() const { return surface_friction_; }

	//amount of friction we experience.
	int friction() const { return friction_; }

	bool on_players_side() const { return on_players_side_; }

	wml::const_node_ptr get_child(const std::string& key) const {
		if(children_.count(key)) {
			return children_.find(key)->second;
		}

		return wml::const_node_ptr();
	}

private:
	std::string id_;
	int hitpoints_;

	typedef std::map<std::string, std::vector<boost::shared_ptr<frame> > > frame_map;
	frame_map frames_;

	boost::shared_ptr<frame> default_frame_;

	game_logic::const_formula_ptr next_animation_formula_;

	event_handler_map event_handlers_;

	int zorder_;

	bool body_harmful_;
	bool body_passthrough_;
	bool ignore_collide_;

	int springiness_;
	int surface_friction_;
	int friction_;

	bool on_players_side_;

	std::map<std::string, wml::const_node_ptr> children_;

	wml::const_node_ptr node_;
};

#endif
