/*
	Copyright (C) 2003-2013 by David White <davewx7@gmail.com>
	
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
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

button::button(const std::string& str, boost::function<void()> onclick)
  : label_(new label(str, graphics::color_white())),
    onclick_(onclick), button_resolution_(BUTTON_SIZE_NORMAL_RESOLUTION),
	button_style_(BUTTON_STYLE_NORMAL), hpadding_(10), vpadding_(4),
	down_(false)
{
	set_environment();
	setup();
}

button::button(widget_ptr label, boost::function<void ()> onclick, BUTTON_STYLE button_style, BUTTON_RESOLUTION button_resolution)
  : label_(label), onclick_(onclick), button_resolution_(button_resolution), button_style_(button_style),
	down_(false), hpadding_(10), vpadding_(4)
	
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

	const variant on_click_value = v["on_click"];
	if(on_click_value.is_function()) {
		ASSERT_LOG(on_click_value.min_function_arguments() == 0, "on_click button function should take 0 arguments: " << v.debug_location());
		static const variant fml("fn()");
		click_handler_.reset(new game_logic::formula(fml));

		game_logic::map_formula_callable* callable = new game_logic::map_formula_callable;
		callable->add("fn", on_click_value);

		handler_arg_.reset(callable);
	} else { 
		click_handler_ = get_environment()->create_formula(on_click_value);
	}

	onclick_ = boost::bind(&button::click, this);
	button_resolution_ = v["resolution"].as_string_default("normal") == "normal" ? BUTTON_SIZE_NORMAL_RESOLUTION : BUTTON_SIZE_DOUBLE_RESOLUTION;
	button_style_ = v["style"].as_string_default("default") == "default" ? BUTTON_STYLE_DEFAULT : BUTTON_STYLE_NORMAL;
	hpadding_ = v["hpad"].as_int(10);
	vpadding_ = v["vpad"].as_int(4);
	if(v.has_key("padding")) {
		ASSERT_LOG(v["padding"].num_elements() == 2, "Incorrect number of padding elements specifed." << v["padding"].num_elements());
		hpadding_ = v["padding"][0].as_int();
		vpadding_ = v["padding"][1].as_int();
	}
	setup();
}

void button::click()
{
	if(handler_arg_) {
		variant value = click_handler_->execute(*handler_arg_);
		get_environment()->execute_command(value);
	} else if(get_environment()) {
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
	if(width() == 0 && height() == 0) {
		set_dim(label_->width()+hpadding_*2,label_->height()+vpadding_*2);
	}
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
#if SDL_VERSION_ATLEAST(2, 0, 0)
	if((event.type == SDL_MOUSEWHEEL) 
#else
	if((event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP) 
		&& (event.button.button == SDL_BUTTON_WHEELUP || event.button.button == SDL_BUTTON_WHEELDOWN)
#endif
		&& in_widget(event.button.x, event.button.y)) {
		// skip processing if mousewheel event
		return claimed;
	}

    if(claimed) {
		current_button_image_set_ = normal_button_image_set_;
		down_ = false;
    }

	if(event.type == SDL_MOUSEMOTION) {
		const SDL_MouseMotionEvent& e = event.motion;
		if(in_widget(e.x,e.y)) {
			current_button_image_set_ = down_ ? depressed_button_image_set_ : focus_button_image_set_;
		} else {
			current_button_image_set_ = normal_button_image_set_;
		}
	} else if(event.type == SDL_MOUSEBUTTONDOWN) {
		const SDL_MouseButtonEvent& e = event.button;
		if(in_widget(e.x,e.y)) {
			current_button_image_set_ = depressed_button_image_set_;
			down_ = true;
			claimed = claim_mouse_events();
		}
	} else if(event.type == SDL_MOUSEBUTTONUP) {
		down_ = false;
		const SDL_MouseButtonEvent& e = event.button;
		if(current_button_image_set_ == depressed_button_image_set_) {
			if(in_widget(e.x,e.y)) {
				current_button_image_set_ = focus_button_image_set_;
				onclick_();
				claimed = claim_mouse_events();
			} else {
				current_button_image_set_ = normal_button_image_set_;
			}
		}
	}
	return claimed;
}

widget_ptr button::get_widget_by_id(const std::string& id)
{
	if(label_ && label_->get_widget_by_id(id)) {
		return label_;
	}
	return widget::get_widget_by_id(id);
}

const_widget_ptr button::get_widget_by_id(const std::string& id) const
{
	if(label_ && label_->get_widget_by_id(id)) {
		return label_;
	}
	return widget::get_widget_by_id(id);
}

std::vector<widget_ptr> button::get_children() const
{
	std::vector<widget_ptr> result;
	result.push_back(label_);
	return result;
}

variant button::get_value(const std::string& key) const
{
	if(key == "label") {
		return variant(label_.get());
	}
	return widget::get_value(key);
}

}
