
/*
   Copyright (C) 2007 by David White <dave@whitevine.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#include "button.hpp"
#include "iphone_controls.hpp"
#include "raster.hpp"
#include "surface_cache.hpp"
#include "framed_gui_element.hpp"

namespace gui {


	
namespace {

int vpadding = 4;
int hpadding = 10;

}

button::button(widget_ptr label, boost::function<void ()> onclick, BUTTON_RESOLUTION button_resolution)
  : label_(label), onclick_(onclick), button_resolution_(button_resolution),
	normal_button_image_set_(framed_gui_element::get("regular_button")),
	depressed_button_image_set_(framed_gui_element::get("regular_button_pressed")),
	focus_button_image_set_(framed_gui_element::get("regular_button_focus")),
	current_button_image_set_(normal_button_image_set_)
	
{
	set_dim(label_->width()+hpadding*2,label_->height()+vpadding*2);
}

bool button::in_button(int xloc, int yloc) const
{
	return xloc > x() && xloc < x() + width() &&
	       yloc > y() && yloc < y() + height();
}

void button::handle_draw() const
{
	label_->set_loc(x()+width()/2 - label_->width()/2,y()+height()/2 - label_->height()/2);
	current_button_image_set_->blit(x(),y(),width(),height(), button_resolution_);
	label_->draw();
}

bool button::handle_event(const SDL_Event& event, bool claimed)
{
    if(claimed) {
		current_button_image_set_ = normal_button_image_set_;
    }

	if(event.type == SDL_MOUSEMOTION) {
		const SDL_MouseMotionEvent& e = event.motion;
		if(current_button_image_set_ == depressed_button_image_set_) {
			//pass
		} else if(in_button(e.x,e.y)) {
			current_button_image_set_ = focus_button_image_set_;
		} else {
			current_button_image_set_ = normal_button_image_set_;
		}
	} else if(event.type == SDL_MOUSEBUTTONDOWN) {
		const SDL_MouseButtonEvent& e = event.button;
		if(in_button(e.x,e.y)) {
			current_button_image_set_ = depressed_button_image_set_;
		}
	} else if(event.type == SDL_MOUSEBUTTONUP) {
		const SDL_MouseButtonEvent& e = event.button;
		if(current_button_image_set_ == depressed_button_image_set_) {
			if(in_button(e.x,e.y)) {
				current_button_image_set_ = focus_button_image_set_;
				onclick_();
				claimed = true;
			} else {
				current_button_image_set_ = normal_button_image_set_;
			}
		}
	}
	return claimed;
}

}
