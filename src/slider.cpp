
/*
 Copyright (C) 2007 by David White <dave@whitevine.net>
 Part of the Silver Tree Project
 
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License version 2 or later.
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY.
 
 See the COPYING file for more details.
 */
#include "slider.hpp"
#include "iphone_controls.hpp"
#include "raster.hpp"
#include "surface_cache.hpp"
#include "gui_section.hpp"

namespace gui {
	
	
	
	slider::slider(int width, boost::function<void (float)> onchange, float position)
	: width_(width), onchange_(onchange), dragging_(false), position_(position),
	slider_left_(gui_section::get("slider_side_left")),
	slider_right_(gui_section::get("slider_side_right")),
	slider_middle_(gui_section::get("slider_middle")),
	slider_button_(gui_section::get("slider_button"))
	{
		set_dim(width_+slider_left_->width()*2, slider_button_->height());
	}
	
	bool slider::in_button(int xloc, int yloc) const
	{
		int button_x = x() + slider_left_->width() + position_*width_;
		return xloc > button_x-40 && xloc < button_x + slider_button_->width()+40 &&
		yloc > y()-10 && yloc < y() + height()+10;
	}
	
	bool slider::in_slider(int xloc, int yloc) const
	{
		return xloc > x() && xloc < x() + width() &&
		yloc > y() && yloc < y() + height();
	}
	
	void slider::handle_draw() const
	{
		int slider_y = y() + height()/2 - slider_middle_->height()/2;
		slider_left_->blit(x(), slider_y);
		slider_middle_->blit(x()+slider_left_->width(), slider_y, width_, slider_middle_->height());
		slider_right_->blit(x()+slider_left_->width()+width_, slider_y);
		slider_button_->blit(x()+slider_left_->width()+position_*width_-slider_button_->width()/2, y());
	}
	
	bool slider::handle_event(const SDL_Event& event, bool claimed)
	{
		if(claimed) {
			dragging_ = false;
		}
		
		if(event.type == SDL_MOUSEMOTION && dragging_) {
			const SDL_MouseMotionEvent& e = event.motion;
			int mouse_x = e.x;
			int mouse_y = e.y;

			int rel_x = mouse_x - x() - slider_left_->width();
			if (rel_x < 0) rel_x = 0;
			if (rel_x > width_) rel_x = width_;
			float pos = (float)rel_x/width_;
			if (pos != position_)
			{
				position_ = pos;
				onchange_(pos);
			}

			return true;
		} else if(event.type == SDL_MOUSEBUTTONDOWN) {
			const SDL_MouseButtonEvent& e = event.button;
			if(in_button(e.x,e.y)) {
				dragging_ = true;
				return true;
			}
		} else if(event.type == SDL_MOUSEBUTTONUP && dragging_) {
			dragging_ = false;
			claimed = true;
		}
		return claimed;
	}
	
}
