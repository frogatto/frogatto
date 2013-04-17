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
#ifndef FORMULA_CALLABLE_DEFINITION_FWD_HPP_INCLUDED
#define FORMULA_CALLABLE_DEFINITION_FWD_HPP_INCLUDED

#include <boost/intrusive_ptr.hpp>

namespace game_logic
{

class formula_callable_definition;

typedef boost::intrusive_ptr<formula_callable_definition> formula_callable_definition_ptr;
typedef boost::intrusive_ptr<const formula_callable_definition> const_formula_callable_definition_ptr;

}

#endif

