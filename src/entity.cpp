#include <iostream>
#include <limits.h>

#include "custom_object.hpp"
#include "entity.hpp"
#include "foreach.hpp"
#include "level.hpp"
#include "playable_custom_object.hpp"
#include "preferences.hpp"
#include "raster.hpp"
#include "solid_map.hpp"
#include "variant_utils.hpp"

entity::entity(variant node)
  : x_(node["x"].as_int()*100),
    y_(node["y"].as_int()*100),
	prev_feet_x_(INT_MIN), prev_feet_y_(INT_MIN),
	last_move_x_(0), last_move_y_(0),
	face_right_(node["face_right"].as_bool()),
	upside_down_(node["upside_down"].as_bool(false)),
	group_(node["group"].as_int(-1)),
    id_(-1), respawn_(node["respawn"].as_bool(true)),
	solid_dimensions_(0), collide_dimensions_(0),
	weak_solid_dimensions_(0), weak_collide_dimensions_(0),
	platform_motion_x_(node["platform_motion_x"].as_int()),
	mouse_over_entity_(false), being_dragged_(false), mouse_button_state_(0)
{
	foreach(bool& b, controls_) {
		b = false;
	}
}

entity::entity(int x, int y, bool face_right)
  : x_(x*100), y_(y*100), prev_feet_x_(INT_MIN), prev_feet_y_(INT_MIN),
	last_move_x_(0), last_move_y_(0),
    face_right_(face_right), upside_down_(false), group_(-1), id_(-1),
	solid_dimensions_(0), collide_dimensions_(0),
	weak_solid_dimensions_(0), weak_collide_dimensions_(0),	platform_motion_x_(0), 
	mouse_over_entity_(false), being_dragged_(false), mouse_button_state_(0)
{
	foreach(bool& b, controls_) {
		b = false;
	}
}

void entity::add_to_level()
{
	last_move_x_ = last_move_y_ = 0;
	prev_feet_x_ = prev_feet_y_ = INT_MIN;
	prev_platform_rect_ = rect();
	calculate_solid_rect();
}

entity_ptr entity::build(variant node)
{
	if(node["is_human"].as_bool()) {
		return entity_ptr(new playable_custom_object(node));
	} else {
		return entity_ptr(new custom_object(node));
	}
}

bool entity::has_feet() const
{
	return solid();
}

int entity::feet_x() const
{
	if(solid_) {
		const int diff = solid_->area().x() + solid_->area().w()/2;
		return face_right() ? x() + diff : x() + current_frame().width() - diff;
	}
	return face_right() ? x() + current_frame().feet_x() : x() + current_frame().width() - current_frame().feet_x();
}

int entity::feet_y() const
{
	if(solid_) {
		return y() + solid_->area().y() + solid_->area().h();
	}
	return y() + current_frame().feet_y();
}

int entity::last_move_x() const
{
	return last_move_x_;
}

int entity::last_move_y() const
{
	return last_move_y_;
}

void entity::set_platform_motion_x(int value)
{
	platform_motion_x_ = value;
}

int entity::map_platform_pos(int xpos) const
{
	if(platform_rect_.w() > 0 && platform_rect_.h() > 0 && xpos >= prev_platform_rect_.x() && xpos < prev_platform_rect_.x() + prev_platform_rect_.w()) {
		const int proportion = xpos - prev_platform_rect_.x();
		int maps_to = (1024*proportion*platform_rect_.w())/prev_platform_rect_.w();
		if(maps_to%1024 >= 512) {
			maps_to = platform_rect_.x() + maps_to/1024 + 1;
		} else {
			maps_to = platform_rect_.x() + maps_to/1024;
		}


		return maps_to - xpos - (feet_x() - prev_feet_x_);
	}

	return 0;
}

int entity::platform_motion_x() const
{
	return platform_motion_x_;
}

void entity::process(level& lvl)
{
	if(prev_feet_x_ != INT_MIN) {
		last_move_x_ = feet_x() - prev_feet_x_;
		last_move_y_ = feet_y() - prev_feet_y_;
	}
	prev_feet_x_ = feet_x();
	prev_feet_y_ = feet_y();
	prev_platform_rect_ = platform_rect_;
}

void entity::set_face_right(bool facing)
{
	if(facing == face_right_) {
		return;
	}
	const int start_x = feet_x();
	face_right_ = facing;
	const int delta_x = feet_x() - start_x;
	x_ -= delta_x*100;
	assert(feet_x() == start_x);

	calculate_solid_rect();
}

void entity::set_upside_down(bool facing)
{
	upside_down_ = facing;
}

void entity::calculate_solid_rect()
{
	const frame& f = current_frame();

	frame_rect_ = rect(x(), y(), f.width(), f.height());
	
	solid_ = calculate_solid();
	if(solid_) {
		const rect& area = solid_->area();

		if(face_right()) {
			solid_rect_ = rect(x() + area.x(), y() + area.y(), area.w(), area.h());
		} else {
			solid_rect_ = rect(x() + f.width() - area.x() - area.w(), y() + area.y(), area.w(), area.h());
		}
	} else {
		solid_rect_ = rect();
	}

	platform_ = calculate_platform();
	if(platform_) {
		const int delta_y = last_move_y();
		const rect& area = platform_->area();
		
		if(area.empty()) {
			platform_rect_ = rect();
		} else {
			if(delta_y < 0) {
				platform_rect_ = rect(x() + area.x(), y() + area.y(), area.w(), area.h() - delta_y);
			} else {
				platform_rect_ = rect(x() + area.x(), y() + area.y(), area.w(), area.h());
			}
		}
	} else {
		platform_rect_ = rect();
	}
}

rect entity::body_rect() const
{
	const frame& f = current_frame();

	const int ypos = y() + (upside_down() ? (f.height() - (f.collide_y() + f.collide_h())) : f.collide_y());
	return rect(face_right() ? x() + f.collide_x() : x() + f.width() - f.collide_x() - f.collide_w(),
	            ypos, f.collide_w(), f.collide_h());
}

rect entity::hit_rect() const
{
	const frame& f = current_frame();
	const std::vector<frame::collision_area>& areas = f.collision_areas();
	foreach(const frame::collision_area& a, areas) {
		if(a.name == "attack") {
			const rect& r = a.area;
			return rect(face_right() ? x() + r.x() : x() + f.width() - r.x() - r.w(), y() + r.y(), r.w(), r.h());
		}
	}

	return rect();
}

point entity::midpoint() const
{
	if(solid()) {
		const rect r = solid_rect();
		return point(r.x() + r.w()/2, r.y() + r.h()/2);
	}

	const frame& f = current_frame();
	return point(x() + f.width()/2, y() + f.height()/2);
}

bool entity::is_alpha(int xpos, int ypos) const
{
	return current_frame().is_alpha(xpos - x(), ypos - y(), time_in_frame(), face_right());
}

void entity::draw_debug_rects() const
{
	if(preferences::show_debug_hitboxes() == false) {
		return;
	}

	const rect& body = solid_rect();
	if(body.w() > 0 && body.h() > 0) {
		const SDL_Rect rect = { body.x(), body.y(), body.w(), body.h() };
		graphics::draw_rect(rect, graphics::color_black(), 0xAA);
	}

	const rect& hit = hit_rect();
	if(hit.w() > 0 && hit.h() > 0) {
		const SDL_Rect rect = { hit.x(), hit.y(), hit.w(), hit.h() };
		graphics::draw_rect(rect, graphics::color_red(), 0xAA);
	}

	const SDL_Rect rect = { feet_x() - 1, feet_y() - 1, 3, 3 };
	graphics::draw_rect(rect, graphics::color_white(), 0xFF);
}

void entity::generate_current(const entity& target, int* velocity_x, int* velocity_y) const
{
	if(current_generator_) {
		const rect& my_rect = body_rect();
		const rect& target_rect = target.body_rect();
		current_generator_->generate(my_rect.mid_x(), my_rect.mid_y(),
		                             target_rect.mid_x(), target_rect.mid_y(), target.mass(),
		                             velocity_x, velocity_y);
	}
}

void entity::add_scheduled_command(int cycle, variant cmd)
{
	scheduled_commands_.push_back(ScheduledCommand(cycle, cmd));
}

std::vector<variant> entity::pop_scheduled_commands()
{
	std::vector<variant> result;
	std::vector<ScheduledCommand>::iterator i = scheduled_commands_.begin();
	while(i != scheduled_commands_.end()) {
		if(--(i->first) <= 0) {
			result.push_back(i->second);
			i = scheduled_commands_.erase(i);
		} else {
			++i;
		}
	}

	return result;
}

void entity::set_current_generator(current_generator* generator)
{
	current_generator_ = current_generator_ptr(generator);
}

void entity::set_attached_objects(const std::vector<entity_ptr>& v)
{
	if(v != attached_objects_) {
		attached_objects_ = v;
	}
}

bool entity::move_centipixels(int dx, int dy)
{
	int start_x = x();
	int start_y = y();
	x_ += dx;
	y_ += dy;
	if(x() != start_x || y() != start_y) {
		calculate_solid_rect();
		return true;
	} else {
		return false;
	}
}

void entity::set_distinct_label()
{
	//generate a random label for the object
	char buf[64];
	sprintf(buf, "_%x", rand());
	set_label(buf);
}

void entity::set_control_status(const std::string& key, bool value)
{
	static const std::string keys[] = { "up", "down", "left", "right", "attack", "jump" };
	const std::string* k = std::find(keys, keys + controls::NUM_CONTROLS, key);
	if(k == keys + controls::NUM_CONTROLS) {
		return;
	}

	const int index = k - keys;
	controls_[index] = value;
}

void entity::read_controls(int cycle)
{
	player_info* info = get_player_info();
	if(info) {
		info->read_controls(cycle);
	}
}

point entity::pivot(const std::string& name, bool reverse_facing) const
{
	const frame& f = current_frame();
	if(name == "") {
		return midpoint();
	}

	bool facing_right = face_right();
	if(reverse_facing) {
		facing_right = !facing_right;
	}

	const point pos = f.pivot(name, time_in_frame());
	if(facing_right) {
		return point(x() + pos.x, y() + pos.y);
	} else {
		return point(x() + f.width() - pos.x, y() + pos.y);
	}
}

void entity::set_spawned_by(const std::string& key)
{
	spawned_by_ = key;
}

const std::string& entity::spawned_by() const
{
	return spawned_by_;
}

bool zorder_compare(const entity_ptr& a, const entity_ptr& b)
{
	//the reverse_global_vertical_zordering flag is set in the player object (our general repository for all major game rules et al).  It's meant to reverse vertical sorting of objects in the same zorder, depending on whether objects are being viewed from above, or below.  In frogatto proper, objects at a higher vertical position should overlap those below.  In a top-down game, the reverse is desirable.
	if(level::current().player() && level::current().player()->reverse_global_vertical_zordering()){
		return a->zorder() < b->zorder() ||
			a->zorder() == b->zorder() && a->zsub_order() < b->zsub_order() ||
			a->zorder() == b->zorder() && a->zsub_order() == b->zsub_order() && a->midpoint().y < b->midpoint().y ||
			a->zorder() == b->zorder() && a->zsub_order() == b->zsub_order() && a->midpoint().y == b->midpoint().y && a->midpoint().x < b->midpoint().x;		
	}
	return a->zorder() < b->zorder() ||
		a->zorder() == b->zorder() && a->zsub_order() < b->zsub_order() ||
		a->zorder() == b->zorder() && a->zsub_order() == b->zsub_order() && a->midpoint().y > b->midpoint().y ||
		a->zorder() == b->zorder() && a->zsub_order() == b->zsub_order() && a->midpoint().y == b->midpoint().y && a->midpoint().x > b->midpoint().x;
}

bool entity_zorder_compare::operator()(const entity_ptr& lhs, const entity_ptr& rhs) 
{
	return zorder_compare(lhs, rhs);
}
