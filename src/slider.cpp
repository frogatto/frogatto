
/*
 Copyright (C) 2007 by David White <dave@whitevine.net>
 Part of the Silver Tree Project
 
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License version 2 or later.
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY.
 
 See the COPYING file for more details.
 */
#include <boost/bind.hpp>

#include "slider.hpp"
#include "image_widget.hpp"
#include "iphone_controls.hpp"
#include "raster.hpp"
#include "surface_cache.hpp"
#include "gui_section.hpp"
#include "widget_factory.hpp"

namespace gui {
	
slider::slider(int width, boost::function<void (double)> onchange, double position)
	: width_(width), onchange_(onchange), dragging_(false), position_(position),
	slider_left_(new gui_section_widget("slider_side_left", -1, -1, 2)),
	slider_right_(new gui_section_widget("slider_side_right", -1, -1, 2)),
	slider_middle_(new gui_section_widget("slider_middle", -1, -1, 2)),
	slider_button_(new gui_section_widget("slider_button", -1, -1, 2))
{
	init();
	set_dim(width_+slider_left_->width()*2, slider_button_->height());
}

slider::slider(const variant& v, const game_logic::formula_callable_ptr& e)
	: widget(v,e), dragging_(false)
{
	onchange_ = boost::bind(&slider::change_delegate, this, _1);
	// XXX replace the 0 with an actual symbol table.
	ffl_handler_ = game_logic::formula_ptr(new game_logic::formula(v["on_change"], 0));

	position_ = v.has_key("position") ? v["position"].as_decimal().as_float() : 0.0;
	
	slider_left_ = v.has_key("slider_left") 
		? widget_factory::create(v["slider_left"], e) 
		: new gui_section_widget("slider_side_left", -1, -1, 2);
	slider_right_ = v.has_key("slider_right") 
		? widget_factory::create(v["slider_right"], e) 
		: new gui_section_widget("slider_side_right", -1, -1, 2);
	slider_middle_ = v.has_key("slider_middle") 
		? widget_factory::create(v["slider_middle"], e) 
		: new gui_section_widget("slider_middle", -1, -1, 2);
	slider_button_ = v.has_key("slider_button") 
		? widget_factory::create(v["slider_button"], e) 
		: new gui_section_widget("slider_button", -1, -1, 2);

	init();
	set_dim(width_+slider_left_->width()*2, slider_button_->height());
}
	
void slider::init() const
{
	int slider_y = y() + height()/2 - slider_middle_->height()/2;
	slider_left_->set_loc(x(), slider_y);
	slider_middle_->set_loc(x()+slider_left_->width(), slider_y);
	slider_middle_->set_dim(width_, slider_middle_->height());
	slider_right_->set_loc(x()+slider_left_->width()+width_, slider_y);
	slider_button_->set_loc(x()+slider_left_->width()+position_*width_-slider_button_->width()/2, y());
}

bool slider::in_button(int xloc, int yloc) const
{
	int button_x = x() + slider_left_->width() + int(position_*width_);
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
	init();
	//int slider_y = y() + height()/2 - slider_middle_->height()/2;
	//slider_left_->blit(x(), slider_y);
	//slider_middle_->blit(x()+slider_left_->width(), slider_y, width_, slider_middle_->height());
	//slider_right_->blit(x()+slider_left_->width()+width_, slider_y);
	//slider_button_->blit(x()+slider_left_->width()+position_*width_-slider_button_->width()/2, y());
	slider_left_->handle_draw();
	slider_middle_->handle_draw();
	slider_right_->handle_draw();
	slider_button_->handle_draw();
}

void slider::change_delegate(double position)
{
	game_logic::map_formula_callable* callable = new game_logic::map_formula_callable(get_environment().get());
	callable->add("position", variant(position));
	variant v(callable);
	ffl_handler_->execute(*callable);
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
		if(ondragend_) {
			const SDL_MouseButtonEvent& e = event.button;
			int mouse_x = e.x;
			int mouse_y = e.y;

			int rel_x = mouse_x - x() - slider_left_->width();
			if (rel_x < 0) rel_x = 0;
			if (rel_x > width_) rel_x = width_;
			float pos = (float)rel_x/width_;
			ondragend_(pos);
		}
	}
	return claimed;
}

void slider::set_value(const std::string& key, const variant& v)
{
	if(key == "position") {
		position_  = v.as_decimal().as_float();
	}
	widget::set_value(key, v);
}

variant slider::get_value(const std::string& key) const
{
	if(key == "position") {
		return variant(position_);
	}
	return widget::get_value(key);
}
	
}
