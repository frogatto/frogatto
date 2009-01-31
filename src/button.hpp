
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef BUTTON_HPP_INCLUDED
#define BUTTON_HPP_INCLUDED

#include <boost/function.hpp>

#include "texture.hpp"
#include "widget.hpp"
#include "framed_gui_element.hpp"


namespace gui {

class button : public widget
{
public:
	button(widget_ptr label, boost::function<void ()> onclick);

private:
	bool in_button(int x, int y) const;
	void handle_draw() const;
	bool handle_event(const SDL_Event& event, bool claimed);

	widget_ptr label_;
	boost::function<void ()> onclick_;
	graphics::texture normal_texture_, focus_texture_,
	                  depressed_texture_;
	graphics::texture* current_texture_;
	const_framed_gui_element_ptr button_image_set_;
};

typedef boost::shared_ptr<button> button_ptr;

}

#endif
