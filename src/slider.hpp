
/*
 Copyright (C) 2010 by David White <dave@whitevine.net>
 Part of the Frogatto Project
 
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License version 2 or later.
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY.
 
 See the COPYING file for more details.
 */
#ifndef SLIDER_HPP_INCLUDED
#define SLIDER_HPP_INCLUDED

#include <boost/function.hpp>

#include "texture.hpp"
#include "widget.hpp"
#include "gui_section.hpp"


namespace gui {
	
	//A slider widget. Forwards to a given function whenever its value changes.
	class slider : public widget
	{
	public:
		slider(int width, boost::function<void (float)> onchange, float position=0.0);
		float position() const {return position_;};
		void set_position (float position) {position_ = position;};
		
	private:
		bool in_slider(int x, int y) const;
		bool in_button(int x, int y) const;
		void handle_draw() const;
		bool handle_event(const SDL_Event& event, bool claimed);
		
		int width_;
		boost::function<void (float)> onchange_;
		bool dragging_;
		float position_;
		
		const_gui_section_ptr slider_left_, slider_right_, slider_middle_, slider_button_;
	};
	
	typedef boost::intrusive_ptr<slider> slider_ptr;
	
}

#endif
