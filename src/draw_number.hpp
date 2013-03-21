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
#ifndef DRAW_NUMBER_HPP_INCLUDED
#define DRAW_NUMBER_HPP_INCLUDED

namespace graphics {
class blit_queue;
}

void queue_draw_number(graphics::blit_queue& q, int number, int places, int xpos, int ypos);
void draw_number(int number, int places, int xpos, int ypos);

#endif
