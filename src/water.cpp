#include <iostream>

#include "foreach.hpp"
#include "formatter.hpp"
#include "raster.hpp"
#include "water.hpp"
#include "wml_node.hpp"
#include "wml_utils.hpp"
#include "color_utils.hpp"


water::water(wml::const_node_ptr node) : level_(wml::get_int(node, "level"))
{
	wml::node::const_child_iterator i1 = node->begin_child("layer");
	wml::node::const_child_iterator i2 = node->end_child("layer");
	for(; i1 != i2; ++i1) {
		zorder_pos pos;
		pos.zorder = wml::get_int(i1->second, "zorder");
		pos.offset = wml::get_int(i1->second, "offset");
		pos.color = string_to_color(node->attr("color"));

		positions_.push_back(pos);
	}
}

wml::node_ptr water::write() const
{
	wml::node_ptr result(new wml::node("water"));
	result->set_attr("level", formatter() << level_);
	foreach(const zorder_pos& pos, positions_) {
		wml::node_ptr node(new wml::node("layer"));
		char buf[128];
		sprintf(buf, "%02x%02x%02x", pos.color.r, pos.color.g, pos.color.b);
		node->set_attr("color", buf);
		node->set_attr("zorder", formatter() << pos.zorder);
		node->set_attr("offset", formatter() << pos.offset);

		result->add_child(node);
	}
	return result;
}

void water::draw(int begin_layer, int end_layer, int x, int y, int w, int h) const
{
	if(begin_layer < min_zorder()) {
		begin_layer = min_zorder();
	}

	if(end_layer > max_zorder()) {
		end_layer = max_zorder();
	}

	if(begin_layer >= end_layer) {
		return;
	}

	const int offset1 = get_offset(begin_layer);
	const int offset2 = get_offset(end_layer);

	const SDL_Rect r = {x, level_ + offset1, w, offset2 - offset1};
	const SDL_Color water_color = get_color(offset1);
	graphics::draw_rect(r, water_color, 128);

	// draw the water edge and the deep, bottom-half-of-the-screen-filling underwater layer
	const int surface = level_ + offset2;
	if(end_layer == max_zorder() && y + w >= surface) {
		const SDL_Rect r = {x, std::max(surface, y), w, h};
		const SDL_Color deepwater_color = {91, 169, 143, 153};
		graphics::draw_rect(r, deepwater_color, 192);
		const SDL_Color waterLine_color = {250, 240, 205, 255};
		const SDL_Rect water_surface_rect = {x, surface, w, 2};
		graphics::draw_rect(water_surface_rect, waterLine_color, 255);
	}
}

SDL_Color water::get_color(int offset) const
{
	int index = 0;
	 for(; index != positions_.size(); ++index) {
		 //loop through till we find the first one with an offset greater than what we're looking at, and return its color
		 if (positions_[index].offset > offset){
			 return positions_[index].color;
		 }
	 }
	return graphics::color_blue();
}

int water::get_offset(int zorder) const
{
	/*int index = 0;
	for(; index != positions_.size(); ++index) {
		if(zorder == positions_[index].zorder) {
			return positions_[index].offset;
		}

		if(zorder < positions_[index].zorder) {
			if(index == 0) {
				return BadOffset;
			}

			const float r1 = zorder - positions_[index-1].zorder;
			const float r2 = positions_[index].zorder - zorder;
			const float offset = (positions_[index-1].offset*r2 + positions_[index].offset*r1)/(r1+r2);
			return static_cast<int>(offset);
		}
	}

	return BadOffset;*/

	
	//normalize the zorder value to a scale from 0 <-> 1, then linearly transform that to an offset
	float zorder_scale = max_zorder() - min_zorder();
	float zorder_current = zorder - min_zorder();
	float zorder_normalized = zorder_current / zorder_scale;
	float offset = zorder_normalized * (max_offset() - min_offset()) + min_offset();
	return static_cast<int>(offset);
}

int water::min_zorder() const
{
	if(positions_.empty()) {
		return BadOffset;
	}

	return positions_.front().zorder;
}

int water::max_zorder() const
{
	if(positions_.empty()) {
		return BadOffset;
	}

	return positions_.back().zorder;
}

int water::min_offset() const
{
	if(positions_.empty()) {
		return BadOffset;
	}
	
	return positions_.front().offset;
}

int water::max_offset() const
{
	if(positions_.empty()) {
		return BadOffset;
	}

	return positions_.back().offset;
}
