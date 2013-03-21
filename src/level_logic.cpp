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
#include <cstdlib>
#include <limits.h>

#include "level.hpp"
#include "level_logic.hpp"

bool cliff_edge_within(const level& lvl, int xpos, int ypos, int deltax)
{
	const int FeetWidth = 5;
	return !lvl.standable(xpos + deltax, ypos) &&
	       !lvl.standable(xpos + deltax, ypos + std::abs(deltax) + FeetWidth);
}

int distance_to_cliff(const level& lvl, int xpos, int ypos, int facing)
{
	const int max_search = 1000;
	const int cliff_face = 5;
	const int cliff_drop = 2;

	bool found = false;
	
	//search for up to three pixels below us to try to get a starting
	//position which is standable.
	for(int n = 0; n != 3; ++n) {
		if(lvl.standable_tile(xpos, ypos)) {
			found = true;
			break;
		}

		++ypos;
	}

	if(!found) {
		return max_search;
	}

	//make sure we are at the surface.
	while(lvl.standable_tile(xpos, ypos-1)) {
		--ypos;
	}

	int result = 0;
	for(; result < max_search; xpos += facing, ++result) {
		if(lvl.standable_tile(xpos, ypos) || lvl.standable_tile(xpos, ypos-1)) {
			int ydiff = 0;
			while(lvl.standable_tile(xpos, ypos-1) && ydiff < cliff_face) {
				--ypos;
				++ydiff;
			}

			if(ydiff == cliff_face) {
				return max_search;
			}
		} else {
			int ydiff = 0;
			while(!lvl.standable_tile(xpos, ypos) && ydiff < cliff_drop) {
				++ypos;
				++ydiff;
			}

			if(ydiff == cliff_drop) {
				return result;
			}
		}
	}

	return result;
}

int find_ground_level(const level& lvl, int xpos, int ypos, int max_search)
{
	if(lvl.standable(xpos, ypos)) {
		--ypos;
		while(lvl.standable(xpos, ypos) && --max_search > 0) {
			--ypos;
		}

		if(!max_search) {
			return INT_MIN;
		}

		return ypos + 1;
	} else {
		//search both up and down, since in the case of a platform the
		//ground may be above us.
		for(int n = 1; n < max_search; ++n) {
			if(lvl.standable(xpos, ypos + n)) {
				return ypos + n - 1;
			}

			if(lvl.standable(xpos, ypos - n) && !lvl.standable(xpos, ypos - n - 1)) {
				return ypos - n;
			}
		}

		return INT_MIN;
	}
}
