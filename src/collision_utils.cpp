#include "collision_utils.hpp"
#include "foreach.hpp"
#include "geometry.hpp"
#include "level.hpp"

bool point_standable(const level& lvl, const entity& e, int x, int y, collision_info* info, ALLOW_PLATFORM allow_platform)
{
	if(lvl.standable(x, y, info ? &info->friction : NULL, info ? &info->traction : NULL, info ? &info->damage : NULL)) {
		return true;
	}

	foreach(const entity_ptr& obj, lvl.get_chars()) {
		if(&e == obj.get()) {
			continue;
		}

		if(allow_platform == SOLID_AND_PLATFORMS) {
			const rect platform_rect = obj->platform_rect();
			if(point_in_rect(point(x,y), platform_rect)) {
				if(info) {
					info->collide_with = obj;
					info->friction = obj->surface_friction();
					info->traction = obj->surface_traction();
					info->adjust_y = y - platform_rect.y();
					info->platform = true;
				}

				return true;
			}
		}

		if(!point_in_rect(point(x,y), obj->solid_rect())) {
			continue;
		}

		const_solid_info_ptr solid = obj->solid();

		if(solid && solid->solid_at(x - obj->x(), y - obj->y(), info ? &info->collide_with_area_id : NULL)) {
			if(info) {
				info->collide_with = obj;
				info->friction = obj->surface_friction();
				info->traction = obj->surface_traction();
			}

			return true;
		}
	}

	return false;
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
