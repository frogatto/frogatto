#include <iostream>

#include "character.hpp"
#include "custom_object.hpp"
#include "entity.hpp"
#include "foreach.hpp"
#include "preferences.hpp"
#include "raster.hpp"
#include "wml_node.hpp"
#include "wml_utils.hpp"

entity::entity(wml::const_node_ptr node)
  : x_(wml::get_int(node, "x")*100),
    y_(wml::get_int(node, "y")*100),
	face_right_(wml::get_bool(node, "face_right")),
	upside_down_(wml::get_bool(node, "upside_down", false)),
	group_(wml::get_int(node, "group", -1)),
    id_(-1), respawn_(wml::get_bool(node, "respawn", true))
{
	foreach(bool& b, controls_) {
		b = false;
	}
}

entity::entity(int x, int y, bool face_right)
  : x_(x*100), y_(y*100), face_right_(face_right), upside_down_(false), group_(-1), id_(-1)
{
	foreach(bool& b, controls_) {
		b = false;
	}
}

entity_ptr entity::build(wml::const_node_ptr node)
{
	if(node->has_attr("is_human")) {
		return entity_ptr(new pc_character(node));
	} else if(node->has_attr("custom")) {
		return entity_ptr(new custom_object(node));
	} else {
		return entity_ptr(new character(node));
	}
}

int entity::feet_x() const
{
	return face_right() ? x() + current_frame().feet_x() : x() + current_frame().width() - current_frame().feet_x();
}

int entity::feet_y() const
{
	return y() + current_frame().feet_y();
}

void entity::set_face_right(bool facing)
{
	if(facing == face_right_) {
		return;
	}
	const int start_x = feet_x();
	face_right_ = facing;
	const int delta_x = feet_x() - start_x;
	x_ += delta_x*100;
}

void entity::set_upside_down(bool facing)
{
	upside_down_ = facing;
}

void entity::activation_distance(int* x, int* y)
{
	*x = 900;
	*y = 500;
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
	if(f.hit(time_in_frame())) {
		return rect(face_right() ? x() + f.hit_x() : x() + f.width() - f.hit_x() - f.hit_w(), y() + f.hit_y(), f.hit_w(), f.hit_h());
	} else {
		return rect();
	}
}

point entity::midpoint() const
{
	const rect& body = body_rect();
	return point(body.mid_x(), body.mid_y());
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

	const rect& body = body_rect();
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
	std::sort(scheduled_commands_.begin(), scheduled_commands_.end());
}

variant entity::get_scheduled_command(int cycle)
{
	if(scheduled_commands_.empty() == false && cycle >= scheduled_commands_.front().first) {
		variant result = scheduled_commands_.front().second;
		scheduled_commands_.erase(scheduled_commands_.begin());
		return result;
	}

	return variant();
}

namespace {
std::vector<const_powerup_ptr> empty_powerup_vector;
}

const std::vector<const_powerup_ptr>& entity::powerups() const
{
	return empty_powerup_vector;
}

const std::vector<const_powerup_ptr>& entity::abilities() const
{
	return empty_powerup_vector;
}

void entity::set_current_generator(current_generator* generator)
{
	current_generator_ = current_generator_ptr(generator);
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
