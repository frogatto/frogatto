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
#include "data_blob.hpp"
#include "image_widget.hpp"
#include "raster.hpp"

#include <iostream>

namespace gui {

image_widget::image_widget(const std::string& fname, int w, int h)
  : texture_(graphics::texture::get(fname)), rotate_(0.0), image_name_(fname)
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
	: widget(v, e)
{
	if(v.has_key("area")) {
		area_ = rect(v["area"]);
	}

	if(v["image"].is_string()) {
		image_name_ = v["image"].as_string();
		texture_ = graphics::texture::get(image_name_);
	} else if(v["image"].is_callable()) {
		data_blob_ptr image_blob = data_blob_ptr(v["image"].try_convert<data_blob>());
		ASSERT_LOG(image_blob != NULL, "Couldn't convert callable in 'image' attribute into a data_blob.");
		image_name_ = (*image_blob)();
		texture_ = graphics::texture::get(image_blob);
	}

	rotate_ = v.has_key("rotation") ? v["rotation"].as_decimal().as_float() : 0.0;
	init(v["image_width"].as_int(-1), v["image_height"].as_int(-1));

	set_claim_mouse_events(v["claim_mouse_events"].as_bool(false));
}

void image_widget::init(int w, int h)
{
	if(w < 0) {
		if(area_.w()) {
			w = area_.w()*2;
		} else {
			w = texture_.width();
		}
	}

	if(h < 0) {
		if(area_.h()) {
			h = area_.h()*2;
		} else {
			h = texture_.height();
		}
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
		if(v.is_string()) {
			image_name_ = v.as_string();
			texture_ = graphics::texture::get(image_name_);
		} else if(v.is_callable()) {
			data_blob_ptr image_blob = data_blob_ptr(v.try_convert<data_blob>());
			ASSERT_LOG(image_blob != NULL, "Couldn't convert callable in 'image' attribute into a data_blob.");
			image_name_ = (*image_blob)();
			texture_ = graphics::texture::get(image_blob);
		}
	} else if(key == "area") {
		area_ = rect(v);
	} else if(key == "rotation") {
		rotate_ = v.as_decimal().as_float();
	} else if(key == "image_wh") {
		ASSERT_LOG(v.is_list() && v.num_elements() == 2, "parameter to 'image_wv' must be two-element list. Found: " << v.to_debug_string());
		init(v[0].as_int(), v[1].as_int());
	}
	return widget::set_value(key, v);
}

variant image_widget::get_value(const std::string& key) const
{
	if(key == "image") {
		return variant(image_name_);
	} else if(key == "area") {
		std::vector<variant> v;
		v.push_back(variant(area_.x()));
		v.push_back(variant(area_.y()));
		v.push_back(variant(area_.x2()));
		v.push_back(variant(area_.y2()));
		return variant(&v);
	} else if(key == "texture") {
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
