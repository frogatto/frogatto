#include <iostream>

#include "character.hpp"
#include "custom_object.hpp"
#include "entity.hpp"
#include "wml_node.hpp"
#include "wml_utils.hpp"

entity::entity(wml::const_node_ptr node)
  : x_(wml::get_int(node, "x")),
    y_(wml::get_int(node, "y")),
	face_right_(wml::get_bool(node, "face_right")),
	group_(wml::get_int(node, "group", -1)),
    id_(-1), respawn_(wml::get_bool(node, "respawn", true))
{
	std::cerr << "entity\n";
}

entity::entity(int x, int y, bool face_right)
  : x_(x), y_(y), face_right_(face_right), group_(-1), id_(-1)
{
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
	x_ += delta_x;
}

void entity::activation_distance(int* x, int* y)
{
	*x = 700;
	*y = 500;
}

rect entity::body_rect() const
{
	const frame& f = current_frame();
	return rect(face_right() ? x() + f.collide_x() : x() + f.width() - f.collide_x() - f.collide_w(), y() + f.collide_y(), f.collide_w(), f.collide_h());
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
