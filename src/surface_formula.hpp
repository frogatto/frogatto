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
#ifndef SURFACE_FORMULA_HPP_INCLUDED
#define SURFACE_FORMULA_HPP_INCLUDED

#include <string>
#include <vector>

#include "graphics.hpp"
#include "surface.hpp"

graphics::surface get_surface_formula(graphics::surface input, const std::string& algo);

GLuint get_gl_shader(const std::vector<std::string>& vertex_shader,
                     const std::vector<std::string>& fragment_shader);

#endif
