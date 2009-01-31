
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#include "button.hpp"
#include "raster.hpp"
#include "surface_cache.hpp"
#include "framed_gui_element.hpp"

namespace gui {

namespace {

int vpadding = 4;
int hpadding = 10;

}

button::button(widget_ptr label, boost::function<void ()> onclick)
  : label_(label), onclick_(onclick),
    normal_texture_(graphics::texture::get("button.png")),
	focus_texture_(graphics::texture::get("button-active.png")),
	depressed_texture_(graphics::texture::get("button-pressed.png")),
	current_texture_(&normal_texture_),
	button_image_set_(new framed_gui_element())
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
	label_->set_loc(x()+hpadding,y()+vpadding);
	//graphics::blit_texture(*current_texture_,x(),y(),width(),height());
	button_image_set_->blit(x(),y(),width(),height());
	label_->draw();
}

bool button::handle_event(const SDL_Event& event, bool claimed)
{
    if(claimed) {
        current_texture_ = &normal_texture_;
    }

	if(event.type == SDL_MOUSEMOTION) {
		const SDL_MouseMotionEvent& e = event.motion;
		if(current_texture_ == &depressed_texture_) {
			//pass
		} else if(in_button(e.x,e.y)) {
			current_texture_ = &focus_texture_;
		} else {
			current_texture_ = &normal_texture_;
		}
	} else if(event.type == SDL_MOUSEBUTTONDOWN) {
		const SDL_MouseButtonEvent& e = event.button;
		if(in_button(e.x,e.y)) {
			current_texture_ = &depressed_texture_;
		}
	} else if(event.type == SDL_MOUSEBUTTONUP) {
		const SDL_MouseButtonEvent& e = event.button;
		if(current_texture_ == &depressed_texture_) {
			if(in_button(e.x,e.y)) {
				current_texture_ = &focus_texture_;
				onclick_();
				claimed = true;
			} else {
				current_texture_ = &normal_texture_;
			}
		}
	}
	return claimed;
}

}
