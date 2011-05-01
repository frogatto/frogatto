#ifndef GAME_REGISTRY_HPP_INCLUDED
#define GAME_REGISTRY_HPP_INCLUDED

#include "formula_callable.hpp"
#include "wml_node_fwd.hpp"

class game_registry : public game_logic::formula_callable {
public:
	static game_registry& instance();
	void set_contents(wml::const_node_ptr node);
	void write_contents(wml::node_ptr node);
private:
	variant get_value(const std::string& key) const;
	void set_value(const std::string& key, const variant& value);

	game_logic::map_formula_callable values_;
};

#endif
