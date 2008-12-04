
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef IMAGE_WIDGET_HPP_INCLUDED
#define IMAGE_WIDGET_HPP_INCLUDED

#include <string>

#include "geometry.hpp"
#include "image_widget_fwd.hpp"
#include "texture.hpp"
#include "widget.hpp"

namespace gui {

class image_widget : public widget
{
public:
	explicit image_widget(const std::string& fname, int w=-1, int h=-1);
	explicit image_widget(graphics::texture tex, int w=-1, int h=-1);

	void set_rotation(GLfloat rotate) { rotate_ = rotate; }
	void set_area(const rect& area) { area_ = area; }

private:
	void handle_draw() const;
	graphics::texture texture_;
	GLfloat rotate_;
	rect area_;
};

}

#endif
