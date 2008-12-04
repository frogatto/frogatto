
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#include "image_widget.hpp"
#include "raster.hpp"

#include <iostream>

namespace gui {

image_widget::image_widget(const std::string& fname, int w, int h)
  : texture_(graphics::texture::get(fname)), rotate_(0.0)
{
	if(w < 0) {
		w = texture_.width();
	}

	if(h < 0) {
		h = texture_.height();
	}

	set_dim(w,h);
}

image_widget::image_widget(graphics::texture tex, int w, int h)
  : texture_(tex), rotate_(0.0)
{
	if(w < 0) {
		w = texture_.width();
	}

	if(h < 0) {
		h = texture_.height();
	}

	set_dim(w,h);
}

void image_widget::handle_draw() const
{
	if(area_.w() == 0) {
		graphics::blit_texture(texture_, x(), y(), width(), height(), rotate_);
	} else {
		graphics::blit_texture(texture_, x(), y(), width(), height(), rotate_,
		                       GLfloat(area_.x())/texture_.width(),
		                       GLfloat(area_.y())/texture_.height(),
		                       GLfloat(area_.x2())/texture_.width(),
		                       GLfloat(area_.y2())/texture_.height());
	}
}

}
