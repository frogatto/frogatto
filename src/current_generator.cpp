#include <iostream>
#include <math.h>

#include "current_generator.hpp"
#include "formatter.hpp"
#include "wml_node.hpp"
#include "wml_utils.hpp"

current_generator_ptr current_generator::create(wml::const_node_ptr node)
{
	const std::string& type = node->attr("type");
	if(type == "radial") {
		return current_generator_ptr(new radial_current_generator(node));
	} else {
		return NULL;
	}
}

current_generator::~current_generator() {
}

variant current_generator::get_value(const std::string& key) const
{
	return variant();
}

radial_current_generator::radial_current_generator(int intensity, int radius)
  : intensity_(intensity), radius_(radius)
{}

radial_current_generator::radial_current_generator(wml::const_node_ptr node)
  : intensity_(wml::get_int(node, "intensity")),
    radius_(wml::get_int(node, "radius"))
{}

void radial_current_generator::generate(int center_x, int center_y, int target_x, int target_y, int* velocity_x, int* velocity_y) {
	if(center_x == target_x && center_y == target_y) {
		return;
	}

	const float xdiff = target_x - center_x;
	const float ydiff = target_y - center_y;
	if(abs(xdiff) >= radius_ || abs(ydiff) > radius_) {
		return;
	}

	const float distance = sqrt(xdiff*xdiff + ydiff*ydiff);
	if(distance >= radius_) {
		return;
	}

	const float intensity = intensity_*(1.0 - distance/radius_);
	const float xdiff_normalized = xdiff/(abs(xdiff) + abs(ydiff));
	const float ydiff_normalized = ydiff/(abs(xdiff) + abs(ydiff));

	std::cerr << "DO_CURRENT: " << center_x << "," << center_y << " ~ " << target_x << "," << target_y << ": "<< intensity << " x " << xdiff_normalized << "," << ydiff_normalized << "\n";
	*velocity_x += xdiff_normalized*intensity;
	*velocity_y += ydiff_normalized*intensity;
}

wml::node_ptr radial_current_generator::write() const
{
	wml::node_ptr result(new wml::node("current_generator"));
	result->set_attr("type", "radial");
	result->set_attr("intensity", formatter() << intensity_);
	result->set_attr("radius", formatter() << radius_);
	return result;
}
