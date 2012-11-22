#ifndef GAME_REGISTRY_HPP_INCLUDED
#define GAME_REGISTRY_HPP_INCLUDED

#include "formula_callable.hpp"
#include "variant.hpp"

class game_registry : public game_logic::formula_callable {
public:
	static game_registry& instance();
	void set_contents(variant node);
	variant write_contents();
private:
	game_registry();
	variant get_value(const std::string& key) const;
	void set_value(const std::string& key, const variant& value);

	variant values_;
};

#endif
