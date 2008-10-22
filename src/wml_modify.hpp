#include <map>
#include <string>

#include "formula.hpp"
#include "wml_node_fwd.hpp"

namespace wml {

class modifier
{
public:
	explicit modifier(wml::const_node_ptr node);
	void modify(wml::node_ptr doc) const;
private:
	struct modification {
		game_logic::const_formula_ptr target;
		std::map<std::string, game_logic::const_formula_ptr> attr;
	};

	std::vector<modification> mods_;
};
}
