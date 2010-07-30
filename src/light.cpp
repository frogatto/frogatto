#include <math.h>

#include "custom_object.hpp"
#include "formatter.hpp"
#include "light.hpp"
#include "wml_node.hpp"
#include "wml_utils.hpp"

light_ptr light::create_light(const custom_object& obj, wml::const_node_ptr node)
{
	if(node->attr("type").str() == "circle") {
		return light_ptr(new circle_light(obj, node));
	} else {
		return light_ptr();
	}
}

light::light(const custom_object& obj) : obj_(obj)
{}

light::~light() {}

circle_light::circle_light(const custom_object& obj, wml::const_node_ptr node)
  : light(obj), radius_(wml::get_int(node, "radius"))
{}

circle_light::circle_light(const custom_object& obj, int radius)
  : light(obj), radius_(radius)
{}

variant light::get_value(const std::string& key) const
{
	return variant();
}

wml::node_ptr circle_light::write() const
{
	wml::node_ptr res(new wml::node("light"));
	res->set_attr("type", "circle");
	res->set_attr("radius", formatter() << radius_);

	return res;
}

void circle_light::process()
{
	center_ = object().midpoint();
}

bool circle_light::on_screen(const rect& screen_area) const
{
	return true;
}

int circle_light::split_strip(const darkness_strip& darkness, darkness_strip* output) const
{
	const rect& area = darkness.area;
	if(area.y() < center_.y - radius_ || area.y() > center_.y + radius_ ||
	   area.x2() < center_.x - radius_ || area.x() > center_.x + radius_) {
		return -1;
	}

	const int width = sqrt(radius_*radius_ - (center_.y - darkness.area.y())*(center_.y - darkness.area.y()));
	int fade_length = 16;

	if(area.x2() < center_.x - width || area.x() > center_.x + width) {
		return -1;
	}

	const int radius_inner = radius_ - fade_length;
	const int width_inner = sqrt(std::max(0, radius_inner*radius_inner - (center_.y - darkness.area.y())*(center_.y - darkness.area.y())));

	const int distance_from_vertical_edge = std::min(area.y() - (center_.y - radius_), (center_.y + radius_) - area.y());

	const int fade_alpha = distance_from_vertical_edge >= fade_length ? 0 : 255*(fade_length - distance_from_vertical_edge)/fade_length;

	fade_length = (width - width_inner);

	const darkness_strip* start_output = output;
	if(area.x() < center_.x - width) {
		output->area = rect(area.x(), area.y(), (center_.x - width) - area.x(), area.h());

		if(darkness.alpha_left != darkness.alpha_right) {
			output->alpha_right = (darkness.alpha_right*output->area.w() + darkness.alpha_left*(darkness.area.w() - output->area.w()))/darkness.area.w();
			output->alpha_left = darkness.alpha_left;
		} else {
			output->alpha_left = output->alpha_right = darkness.alpha_left;
		}

		++output;

		if(fade_length <= darkness.area.w()) {
			output->alpha_left = darkness.alpha_right;
			output->alpha_right = (fade_alpha*darkness.alpha_right)/255;
			output->area = rect(center_.x - width, area.y(), fade_length, area.h());
			++output;
		}
	}

	if(area.x2() > center_.x + width) {
		output->area = rect(center_.x + width, area.y(), area.x2() - (center_.x + width), area.h());

		if(darkness.alpha_left != darkness.alpha_right) {
			output->alpha_left = (darkness.alpha_left*output->area.w() + darkness.alpha_right*(darkness.area.w() - output->area.w()))/darkness.area.w();
			output->alpha_right = darkness.alpha_right;
		} else {
			output->alpha_left = output->alpha_right = darkness.alpha_left;
		}

		++output;

		if(fade_length <= darkness.area.w()) {
			output->alpha_left = (fade_alpha*darkness.alpha_left)/255;
			output->alpha_right = darkness.alpha_left;
			output->area = rect((center_.x + width) - fade_length, area.y(), fade_length, area.h());
			++output;
		}
	}

	return output - start_output;
}
