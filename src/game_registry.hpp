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
