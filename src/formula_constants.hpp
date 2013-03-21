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
#ifndef FORMULA_CONSTANTS_HPP_INCLUDED
#define FORMULA_CONSTANTS_HPP_INCLUDED

#include "variant.hpp"

#include <string>

namespace game_logic
{

variant get_constant(const std::string& id);

class constants_loader
{
public:
	explicit constants_loader(variant node);
	~constants_loader();

	bool same_as_base() const { return same_as_base_; }

private:
	bool same_as_base_;
};

}

#endif
