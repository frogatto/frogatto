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
		std::vector<std::string> add_children, add_if_not_present_children;
		std::vector<std::string> remove_children;
		std::map<std::string, game_logic::const_formula_ptr> attr;
	};

	static void modify_target(variant target, const modification& mod);

	std::vector<modification> mods_;
};
}
