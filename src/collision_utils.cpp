#include "collision_utils.hpp"
#include "foreach.hpp"
#include "geometry.hpp"
#include "level.hpp"

bool point_standable(level& lvl, int x, int y, collision_info* info)
{
	
}

bool entity_collides(level& lvl, const entity& e, MOVE_DIRECTION dir, collision_info* info)
{
	if(entity_collides_with_level(lvl, e, dir, info ? &info->friction : NULL, info ? &info->traction : NULL, info ? &info->damage : NULL)) {
		return true;
	}

	foreach(const entity_ptr& obj, lvl.get_chars()) {
		if(obj.get() != &e && entity_collides_with_entity(e, *obj, info ? &info->area_id : NULL, info ? &info->collide_with_area_id : NULL)) {
			return true;
		}
	}

	return false;
}

bool entity_collides_with_entity(const entity& e, const entity& other, const std::string** area_id, const std::string** other_area_id)
{
	const rect our_rect = e.solid_rect();
	const rect other_rect = other.solid_rect();

	if(!rects_intersect(our_rect, other_rect)) {
		return false;
	}

	const rect area = intersection_rect(our_rect, other_rect);

	const_solid_info_ptr our_solid = e.solid();
	const_solid_info_ptr other_solid = other.solid();
	assert(our_solid && other_solid);

	for(int y = area.y(); y <= area.y2(); ++y) {
		for(int x = area.x(); x < area.x2(); ++x) {
			const int our_x = x - e.x();
			const int our_y = y - e.y();
			if(our_solid->solid_at(our_x, our_y, area_id)) {
				const int other_x = x - other.x();
				const int other_y = y - other.y();
				if(other_solid->solid_at(other_x, other_y, other_area_id)) {
					return true;
				}
			}
		}
	}

	return false;
}

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
