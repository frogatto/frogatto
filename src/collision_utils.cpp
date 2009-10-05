#include "collision_utils.hpp"
#include "foreach.hpp"
#include "level.hpp"

bool entity_collides_with_level(const level& lvl, const entity& e, MOVE_DIRECTION dir, int* friction, int* traction, int* damage)
{
	const_solid_info_ptr s = e.solid();
	if(!s) {
		return false;
	}
	foreach(const const_solid_map_ptr& m, s->solid()) {
		const std::vector<point>& points = m->dir(dir);
		foreach(const point& p, points) {
			if(lvl.solid(e.x() + p.x, e.y() + p.y, friction, traction, damage)) {
				return true;
			}
		}
	}

	return false;
}

int entity_collides_with_level_count(const level& lvl, const entity& e, MOVE_DIRECTION dir)
{
	const_solid_info_ptr s = e.solid();
	if(!s) {
		return 0;
	}

	int count = 0;
	foreach(const const_solid_map_ptr& m, s->solid()) {
		const std::vector<point>& points = m->dir(dir);
		foreach(const point& p, points) {
			if(lvl.solid(e.x() + p.x, e.y() + p.y)) {
				std::cerr << "COLLIDE: " << p.x << "," << p.y << "\n";
				++count;
			}
		}
	}

	return count;
}
