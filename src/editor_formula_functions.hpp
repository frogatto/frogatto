/*
	Copyright (C) 2003-2013 by David White <davewx7@gmail.com>
	
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
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
