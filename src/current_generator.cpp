#include <iostream>
#include <math.h>

#include "current_generator.hpp"
#include "formatter.hpp"
#include "variant.hpp"
#include "variant_utils.hpp"

current_generator_ptr current_generator::create(variant node)
{
	const std::string& type = node["type"].as_string();
	if(type == "radial") {
		return current_generator_ptr(new radial_current_generator(node));
	} else if(type == "rect") {
		return current_generator_ptr(new rect_current_generator(node));
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

radial_current_generator::radial_current_generator(variant node)
  : intensity_(node["intensity"].as_int()),
    radius_(node["radius"].as_int())
{}

void radial_current_generator::generate(int center_x, int center_y, int target_x, int target_y, int target_mass, int* velocity_x, int* velocity_y) {
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

variant radial_current_generator::write() const
{
	variant_builder result;
	result.add("type", "radial");
	result.add("intensity", formatter() << intensity_);
	result.add("radius", formatter() << radius_);
	return result.build();
}

rect_current_generator::rect_current_generator(const rect& r, int xvelocity, int yvelocity, int strength)
  : rect_(r), xvelocity_(xvelocity), yvelocity_(yvelocity), strength_(strength)
{}

rect_current_generator::rect_current_generator(variant node)
  : rect_(node["rect"].as_string()), xvelocity_(node["xvelocity"].as_int()), yvelocity_(node["yvelocity"].as_int()), strength_(node["strength"].as_int())
{}

void rect_current_generator::generate(int center_x, int center_y, int target_x, int target_y, int target_mass, int* velocity_x, int* velocity_y)
{
	const int strength = strength_;
	if(point_in_rect(point(target_x, target_y), rect_)) {
		if(xvelocity_ > 0 && *velocity_x < xvelocity_) {
			int amount = (xvelocity_ - std::max(0, *velocity_x))*strength/(target_mass*1000);
			const int distance = rect_.x2() - target_x;
			amount = (amount*distance*distance)/(rect_.h()*rect_.h());
			*velocity_x += amount;
			if(*velocity_x > xvelocity_) {
				*velocity_x = xvelocity_;
			}
		} else if(xvelocity_ < 0 && *velocity_x > xvelocity_) {
			int amount = (xvelocity_ - std::min(0, *velocity_x))*strength/(target_mass*1000);
			const int distance = target_x - rect_.x();
			amount = (amount*distance*distance)/(rect_.h()*rect_.h());
			*velocity_x += amount;
			if(*velocity_x < xvelocity_) {
				*velocity_x = xvelocity_;
			}
		}

		if(yvelocity_ > 0 && *velocity_y < yvelocity_) {
			int amount = (yvelocity_ - std::max(0, *velocity_y))*strength/(target_mass*1000);
			const int distance = rect_.y2() - target_y;
			amount = (amount*distance*distance)/(rect_.h()*rect_.h());
			*velocity_y += amount;
			if(*velocity_y > yvelocity_) {
				*velocity_y = yvelocity_;
			}
		} else if(yvelocity_ < 0 && *velocity_y > yvelocity_) {
			int amount = yvelocity_*strength/(target_mass*1000);
			const int distance = target_y - rect_.y();
//			amount = (amount*distance*distance)/(rect_.h()*rect_.h());
			std::cerr << "DIST: " << distance << "/" << rect_.h() << " " << *velocity_y << "\n";
			if(distance < rect_.h()/2 && *velocity_y > 0) {
				std::cerr << "CANCEL\n";
				amount = 0;
			}
			*velocity_y += amount;
			if(*velocity_y < yvelocity_) {
//				*velocity_y = yvelocity_;
			}
		}
	}
}

variant rect_current_generator::write() const
{
	variant_builder node;
	node.add("type", "rect");
	node.add("rect", rect_.to_string());
	node.add("xvelocity", formatter() << xvelocity_);
	node.add("yvelocity", formatter() << yvelocity_);
	node.add("strength", formatter() << strength_);
	return node.build();
}
