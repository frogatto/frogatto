#ifndef ITEM_TYPE_HPP_INCLUDED
#define ITEM_TYPE_HPP_INCLUDED

#include <map>
#include <string>

#include "boost/shared_ptr.hpp"

#include "formula.hpp"
#include "frame.hpp"
#include "wml_node.hpp"

class item_type;

typedef boost::shared_ptr<item_type> item_type_ptr;
typedef boost::shared_ptr<const item_type> const_item_type_ptr;

class item_type
{
public:
	static void init(wml::const_node_ptr node);
	static const_item_type_ptr get(const std::string& id);

	explicit item_type(wml::const_node_ptr node);
	const std::string& id() const { return id_; }
	const frame& get_frame() const { return frame_; }
	const frame* touched_frame() const;
	bool destroy_on_touch() const { return destroy_on_touch_; }
	bool automatic_touch() const { return automatic_touch_; }

	const std::map<std::string, game_logic::const_formula_ptr>& on_touch() const { return on_touch_; }
	game_logic::const_formula_ptr touch_condition() const { return touch_condition_; }

	const std::string& on_touch_particles() const { return on_touch_particles_; }
	int num_on_touch_particles() const { return num_on_touch_particles_; }

private:
	std::string id_;
	frame frame_;
	boost::shared_ptr<frame> touched_frame_;
	bool destroy_on_touch_;
	bool automatic_touch_;
	std::map<std::string, game_logic::const_formula_ptr> on_touch_;
	game_logic::const_formula_ptr touch_condition_;
	std::string on_touch_particles_;
	int num_on_touch_particles_;
};

#endif
