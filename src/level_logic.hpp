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
#ifndef LEVEL_LOGIC_HPP_INCLUDED
#define LEVEL_LOGIC_HPP_INCLUDED

class level;

// find out if [xpos + deltax, ypos] is over a drop-off from [xpos, ypos].
// [xpos, ypos] must be on the ground. deltax must not be greater than
// the tile size.
bool cliff_edge_within(const level& lvl, int xpos, int ypos, int deltax);

// find out how far the nearest cliff is from [xpos, ypos]
int distance_to_cliff(const level& lvl, int xpos, int ypos, int facing);

// given a position, will return the xpos of the ground level closest to this
// position. Will search downwards if (xpos,ypos) is not solid, and upwards
// if (xpos,ypos) is solid. Will return INT_MIN on failure to find a result.
int find_ground_level(const level& lvl, int xpos, int ypos, int max_search=20);

#endif
