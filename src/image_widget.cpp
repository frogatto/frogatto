
/*
   Copyright (C) 2007 by David White <dave@whitevine.net>
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
	set_environment();
	init(w, h);
}

image_widget::image_widget(graphics::texture tex, int w, int h)
  : texture_(tex), rotate_(0.0)
{
	set_environment();
	init(w, h);
}

image_widget::image_widget(const variant& v, game_logic::formula_callable* e)
{
	set_environment();
	texture_ = graphics::texture::get(v["image"].as_string());
	rotate_ = v.has_key("rotation") ? v["rotation"].as_decimal().as_float() : 0.0;
	init(v["image_width"].as_int(), v["image_height"].as_int());
}

void image_widget::init(int w, int h)
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

void image_widget::set_value(const std::string& key, const variant& v)
{
	if(key == "image") {
		texture_ = graphics::texture::get(v.as_string());
	} else if(key == "rotation") {
		rotate_ = v.as_decimal().as_float();
	}
	return widget::set_value(key, v);
}

variant image_widget::get_value(const std::string& key) const
{
	if(key == "image") {
		return variant(texture_.get_id());
	} else if(key == "rotation") {
		return variant(rotate_);
	} else if(key == "image_width") {
		return variant(texture_.width());
	} else if(key == "image_height") {
		return variant(texture_.height());
	}
	return widget::get_value(key);
}

gui_section_widget::gui_section_widget(const std::string& id, int w, int h, int scale)
  : section_(gui_section::get(id)), scale_(scale)
{
	set_environment();
	if(section_ && w == -1) {
		set_dim((section_->width()/2)*scale_, (section_->height()/2)*scale_);
	} else {
		set_dim(w,h);
	}
}

gui_section_widget::gui_section_widget(const variant& v, game_logic::formula_callable* e) 
	: widget(v,e)
{
	section_ = gui_section::get(v);
	scale_ = v["scale"].as_int(1);
	if(!v.has_key("width") && section_) {
		set_dim((section_->width()/2)*scale_, (section_->height()/2)*scale_);
	}
}

void gui_section_widget::set_gui_section(const std::string& id)
{
	section_ = gui_section::get(id);
}

void gui_section_widget::handle_draw() const
{
	if(section_) {
		section_->blit(x(), y(), width(), height());
	}
}

void gui_section_widget::set_value(const std::string& key, const variant& v)
{
	if(key == "name") {
		set_gui_section(v.as_string());
	} else if(key == "scale" && section_) {
		set_dim((section_->width()/2)*scale_, (section_->height()/2)*scale_);
	}
	widget::set_value(key, v);
}

variant gui_section_widget::get_value(const std::string& key) const
{
	if(key == "scale") {
		return variant(scale_);
	}
	return widget::get_value(key);
}

}
