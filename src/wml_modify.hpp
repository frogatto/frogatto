#include <map>
#include <string>

#include "formula.hpp"
#include "variant.hpp"

namespace wml {

class modifier
{
public:
	explicit modifier(variant node);
	variant modify(variant doc) const;
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
