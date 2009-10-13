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

		const frame& f = obj->current_frame();
		const int xpos = obj->face_right() ? x - obj->x() : obj->x() + f.width() - x - 1;

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

	const frame& our_frame = e.current_frame();
	const frame& other_frame = other.current_frame();

	for(int y = area.y(); y <= area.y2(); ++y) {
		for(int x = area.x(); x < area.x2(); ++x) {
			const int our_x = e.face_right() ? x - e.x() : (e.x() + our_frame.width()-1) - x;
			const int our_y = y - e.y();
			if(our_solid->solid_at(our_x, our_y, area_id)) {
				const int other_x = other.face_right() ? x - other.x() : (other.x() + other_frame.width()-1) - x;
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
	if(e.face_right() == false) {
		if(dir == MOVE_RIGHT) {
			dir = MOVE_LEFT;
		} else if(dir == MOVE_LEFT) {
			dir = MOVE_RIGHT;
		}
	}

	const_solid_info_ptr s = e.solid();
	if(!s) {
		return false;
	}

	const frame& f = e.current_frame();
	foreach(const const_solid_map_ptr& m, s->solid()) {
		const std::vector<point>& points = m->dir(dir);
		foreach(const point& p, points) {
			const int xpos = e.face_right() ? e.x() + p.x : e.x() + f.width() - 1 - p.x;
			if(lvl.solid(xpos, e.y() + p.y, friction, traction, damage)) {
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

	std::cerr << "FEET_X: " << e.feet_x() << "\n";

	const frame& f = e.current_frame();
	int count = 0;
	foreach(const const_solid_map_ptr& m, s->solid()) {
		const std::vector<point>& points = m->dir(dir);
		foreach(const point& p, points) {
			const int xpos = e.face_right() ? e.x() + p.x : e.x() + f.width() - 1 - p.x;
			std::cerr << "COLLIDE: (" << xpos << "," << (e.y() + p.y) << ") " << (lvl.solid(xpos, e.y() + p.y) ? "1" : "0") << "\n";
			if(lvl.solid(xpos, e.y() + p.y)) {
				++count;
			}
		}
	}

	return count;
}

int entity_user_collision(const entity& a, const entity& b, collision_pair* areas_colliding, int buf_size)
{
	if(&a == &b) {
		return 0;
	}

	const frame& fa = a.current_frame();
	const frame& fb = b.current_frame();

	if(fa.collision_areas().empty() || fb.collision_areas().empty()) {
		return 0;
	}

	if(!rects_intersect(rect(a.x(), a.y(), fa.width(), fa.height()),
	                    rect(b.x(), b.y(), fb.width(), fb.height()))) {
		return 0;
	}

	int result = 0;

	foreach(const frame::collision_area& area_a, fa.collision_areas()) {
		foreach(const frame::collision_area& area_b, fb.collision_areas()) {
			rect rect_a(a.face_right() ? a.x() + area_a.area.x() : a.x() + fa.width() - area_a.area.x() - area_a.area.w(),
			            a.y() + area_a.area.y(),
						area_a.area.w(), area_a.area.h());
			rect rect_b(b.face_right() ? b.x() + area_b.area.x() : b.x() + fb.width() - area_b.area.x() - area_b.area.w(),
			            b.y() + area_b.area.y(),
						area_b.area.w(), area_b.area.h());
			if(rects_intersect(rect_a, rect_b)) {
				const int time_a = a.time_in_frame();
				const int time_b = b.time_in_frame();

				bool found = false;
				const rect intersection = intersection_rect(rect_a, rect_b);
				for(int y = intersection.y(); y <= intersection.y2() && !found; ++y) {
					for(int x = intersection.x(); x <= intersection.x2(); ++x) {
						if(!fa.is_alpha(x - a.x(), y - a.y(), time_a, a.face_right()) &&
						   !fb.is_alpha(x - b.x(), y - b.y(), time_b, b.face_right())) {
							found = true;
							break;
						}
					}
				}

				if(found) {
					++result;
					if(buf_size > 0) {
						areas_colliding->first = &area_a.name;
						areas_colliding->second = &area_b.name;
						++areas_colliding;
						--buf_size;
					}
				}
			}
		}
	}

	return result;
}

namespace {
class user_collision_callable : public game_logic::formula_callable {
	entity_ptr a_, b_;
	const std::string* area_a_;
	const std::string* area_b_;
public:
	user_collision_callable(entity_ptr a, entity_ptr b, const std::string& area_a, const std::string& area_b) : a_(a), b_(b), area_a_(&area_a), area_b_(&area_b) {
	}

	variant get_value(const std::string& key) const {
		if(key == "collide_with") {
			return variant(b_.get());
		} else if(key == "area") {
			return variant(*area_a_);
		} else if(key == "collide_with_area") {
			return variant(*area_b_);
		} else {
			return variant();
		}
	}
};
}

void detect_user_collisions(level& lvl)
{
	const int MaxCollisions = 16;
	collision_pair collision_buf[MaxCollisions];
	for(std::vector<entity_ptr>::const_iterator i = lvl.get_chars().begin(); i != lvl.get_chars().end(); ++i) {
		for(std::vector<entity_ptr>::const_iterator j = i + 1; j != lvl.get_chars().end(); ++j) {
			const entity_ptr& a = *i;
			const entity_ptr& b = *j;
			int ncollisions = entity_user_collision(*a, *b, collision_buf, MaxCollisions);
			if(ncollisions > MaxCollisions) {
				ncollisions = MaxCollisions;
			}

			for(int n = 0; n != ncollisions; ++n) {
				{
					user_collision_callable* callable = new user_collision_callable(a, b, *collision_buf[n].first, *collision_buf[n].second);
					game_logic::formula_callable_ptr ptr(callable);
					a->handle_event("collide_object", callable);
					a->handle_event("collide_object_" + *collision_buf[n].first, callable);
				}

				{
					user_collision_callable* callable = new user_collision_callable(b, a, *collision_buf[n].second, *collision_buf[n].first);
					game_logic::formula_callable_ptr ptr(callable);
					b->handle_event("collide_object", callable);
					b->handle_event("collide_object_" + *collision_buf[n].second, callable);
				}
			}
		}
	}
}
