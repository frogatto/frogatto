
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

#include "asserts.hpp"
#include "button.hpp"
#include "custom_object_functions.hpp"
#include "iphone_controls.hpp"
#include "formula.hpp"
#include "label.hpp"
#include "raster.hpp"
#include "surface_cache.hpp"
#include "framed_gui_element.hpp"
#include "widget_factory.hpp"

namespace gui {

namespace {

int vpadding = 4;
int hpadding = 10;

}

button::button(const std::string& str, boost::function<void()> onclick)
  : label_(new label(str, graphics::color_white())),
    onclick_(onclick), button_resolution_(BUTTON_SIZE_NORMAL_RESOLUTION),
	button_style_(BUTTON_STYLE_NORMAL),
	down_(false)
{
	set_environment();
	setup();
}

button::button(widget_ptr label, boost::function<void ()> onclick, BUTTON_STYLE button_style, BUTTON_RESOLUTION button_resolution)
  : label_(label), onclick_(onclick), button_resolution_(button_resolution), button_style_(button_style),
	down_(false)
	
{
	set_environment();
	setup();
}

button::button(const variant& v, game_logic::formula_callable* e) : widget(v,e), down_(false)
{
	variant label_var = v["label"];
	label_ = label_var.is_map() ? widget_factory::create(label_var, e) : new label(label_var.as_string_default("Button"), graphics::color_white());
	ASSERT_LOG(v.has_key("on_click"), "Button must be supplied with an on_click handler");
	// create delegate for onclick
	ASSERT_LOG(get_environment() != 0, "You must specify a callable environment");
	click_handler_ = get_environment()->create_formula(v["on_click"]);
	onclick_ = boost::bind(&button::click, this);
	button_resolution_ = v["resolution"].as_string_default("normal") == "normal" ? BUTTON_SIZE_NORMAL_RESOLUTION : BUTTON_SIZE_DOUBLE_RESOLUTION;
	button_style_ = v["style"].as_string_default("default") == "default" ? BUTTON_STYLE_DEFAULT : BUTTON_STYLE_NORMAL;
	setup();
}

void button::click()
{
	if(get_environment()) {
		variant value = click_handler_->execute(*get_environment());
		get_environment()->execute_command(value);
	} else {
		std::cerr << "button::click() called without environment!" << std::endl;
	}
}

void button::setup()
{
	if(button_style_ == BUTTON_STYLE_DEFAULT){
		normal_button_image_set_ = framed_gui_element::get("default_button");
		depressed_button_image_set_ = framed_gui_element::get("default_button_pressed");
		focus_button_image_set_ = framed_gui_element::get("default_button_focus");
	}else{
		normal_button_image_set_ = framed_gui_element::get("regular_button");
		depressed_button_image_set_ = framed_gui_element::get("regular_button_pressed");
		focus_button_image_set_ = framed_gui_element::get("regular_button_focus");
	}
	current_button_image_set_ = normal_button_image_set_;
	
	set_label(label_);
}

void button::set_label(widget_ptr label)
{
	label_ = label;
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
	current_button_image_set_->blit(x(),y(),width(),height(), button_resolution_ != 0);
	label_->draw();
}

void button::handle_process()
{
	widget::handle_process();
	label_->process();
}

bool button::handle_event(const SDL_Event& event, bool claimed)
{
	if((event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP) 
		&& (event.button.button == SDL_BUTTON_WHEELUP || event.button.button == SDL_BUTTON_WHEELDOWN)
		&& in_button(event.button.x, event.button.y)) {
		// skip processing if mousewheel event
		return claimed;
	}

    if(claimed) {
		current_button_image_set_ = normal_button_image_set_;
		down_ = false;
    }

	if(event.type == SDL_MOUSEMOTION) {
		const SDL_MouseMotionEvent& e = event.motion;
		if(in_button(e.x,e.y)) {
			current_button_image_set_ = down_ ? depressed_button_image_set_ : focus_button_image_set_;
		} else {
			current_button_image_set_ = normal_button_image_set_;
		}
	} else if(event.type == SDL_MOUSEBUTTONDOWN) {
		const SDL_MouseButtonEvent& e = event.button;
		if(in_button(e.x,e.y)) {
			current_button_image_set_ = depressed_button_image_set_;
			down_ = true;
			claimed = true;
		}
	} else if(event.type == SDL_MOUSEBUTTONUP) {
		down_ = false;
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

variant button::get_value(const std::string& key) const
{
	if(key == "label") {
		label_->get_value(key);
	}
	return widget::get_value(key);
}

}
