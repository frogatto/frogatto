#include <iostream>

#include "foreach.hpp"
#include "formatter.hpp"
#include "raster.hpp"
#include "water.hpp"
#include "wml_node.hpp"
#include "wml_utils.hpp"

water::water(wml::const_node_ptr node) : level_(wml::get_int(node, "level"))
{
	wml::node::const_child_iterator i1 = node->begin_child("layer");
	wml::node::const_child_iterator i2 = node->end_child("layer");
	for(; i1 != i2; ++i1) {
		zorder_pos pos;
		pos.zorder = wml::get_int(i1->second, "zorder");
		pos.offset = wml::get_int(i1->second, "offset");
		positions_.push_back(pos);
	}
}

wml::node_ptr water::write() const
{
	wml::node_ptr result(new wml::node("water"));
	result->set_attr("level", formatter() << level_);
	foreach(const zorder_pos& pos, positions_) {
		wml::node_ptr node(new wml::node("layer"));
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
	graphics::draw_rect(r, graphics::color_blue(), 128);

	const int surface = level_ + offset2;
	if(end_layer == max_zorder() && y + w >= surface) {
		const SDL_Rect r = {x, std::max(surface, y), w, h};
		graphics::draw_rect(r, graphics::color_blue(), 192);
		const SDL_Rect water_surface_rect = {x, surface, w, 2};
		graphics::draw_rect(water_surface_rect, graphics::color_white(), 255);
	}
}

int water::get_offset(int zorder) const
{
	int index = 0;
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

	return BadOffset;
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
