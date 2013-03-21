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
#ifndef RECTANGLE_ROTATOR_HPP_INCLUDED
#define RECTANGLE_ROTATOR_HPP_INCLUDED

#include "geometry.hpp"

void rotate_rect(GLshort center_x, GLshort center_y, float rotation, GLshort* rect_vertexes);
void rotate_rect(const rect& r, GLfloat angle, GLshort* output);
point rotate_point_around_origin_with_offset(int x1, int y1, float alpha, int u1, int v1);
point rotate_point_around_origin(int x1, int y1, float alpha);

#endif
