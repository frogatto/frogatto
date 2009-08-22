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
	const int drop = 5; //arbitrary value considered to qualify something as a cliff
						//TODO:  make this somehow related to our actual values for being able to scale height differences
	const int max_search = 1000;
	
	for( int ix = xpos; abs(ix-xpos) < max_search; ix += facing) {
		if( !lvl.standable_tile( ix, ypos) ){
			//then check that column to see if there is a standable pixel within a short height difference
			int found_standable = 0;
			for(int iy = ypos-drop; iy < ypos + 2*drop; ++iy){ 
				found_standable += lvl.standable_tile( ix, iy);
			}
			if( !found_standable){
				return abs(ix-xpos);
			}
		}
	}
	return max_search;
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
		++ypos;
		while(!lvl.standable(xpos, ypos) && --max_search > 0) {
			++ypos;
		}

		if(!max_search) {
			return INT_MIN;
		}

		return ypos - 1;
	}
}
