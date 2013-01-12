#ifndef EDITOR_FORMULA_FUNCTIONS_HPP_INCLUDED
#define EDITOR_FORMULA_FUNCTIONS_HPP_INCLUDED
#ifndef NO_EDITOR

#include <string>
#include <vector>

class editor;

namespace editor_script {

struct info {
	std::string name;
};

std::vector<info> all_scripts();

void execute(const std::string& id, editor& e);

}

#endif // !NO_EDITOR
#endif
