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
#ifndef PREPROCESSOR_HPP_INCLUDED
#define PREPROCESSOR_HPP_INCLUDED

#include <string>

#include "variant.hpp"

namespace game_logic {
	class formula_callable;
}

std::string preprocess(const std::string& input);

struct preprocessor_error {};
variant preprocess_string_value(const std::string& input, const game_logic::formula_callable* callable=NULL);


#endif
