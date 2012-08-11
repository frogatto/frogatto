
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
#include "preferences.hpp"
#include "raster.hpp"
#include "tooltip.hpp"
#include "i18n.hpp"
#include "widget.hpp"
#include "iphone_controls.hpp"

#include <iostream>

namespace gui {

widget::widget(const variant& v, game_logic::formula_callable* e) 
	: environ_(e), w_(0), h_(0), x_(0), y_(0), zorder_(0), 
	true_x_(0), true_y_(0), disabled_(false), disabled_opacity_(v["disabled_opacity"].as_int(127)),
	tooltip_displayed_(false), id_(v["id"].as_string_default()),
	align_h_(HALIGN_LEFT), align_v_(VALIGN_TOP)
{
	if(v.has_key("width")) {
		w_ = v["width"].as_int();
	} 
	if(v.has_key("height")) {
		h_ = v["height"].as_int();
	} 
	if(v.has_key("wh")) {
		std::vector<int> iv = v["wh"].as_list_int();
		ASSERT_LOG(iv.size() == 2, "WH attribute must be 2 integer elements.");
		w_ = iv[0];
		h_ = iv[1];
	}
	if(v.has_key("rect")) {
		std::vector<int> r = v["rect"].as_list_int();
		ASSERT_LOG(r.size() == 4, "Four values must be supplied to the rect attribute");
		set_loc(r[0], r[1]);
		set_dim(r[2], r[3]);
	} 
	if(v.has_key("draw_area")) {
		std::vector<int> r = v["draw_area"].as_list_int();
		ASSERT_LOG(r.size() == 4, "Four values must be supplied to the rect attribute");
		set_loc(r[0], r[1]);
		set_dim(r[2], r[3]);
	} 
	if(v.has_key("x")) {
		true_x_ = x_ = v["x"].as_int();
	} 
	if(v.has_key("y")) {
		true_y_ = y_ = v["y"].as_int();
	}
	if(v.has_key("xy")) {
		std::vector<int> iv = v["xy"].as_list_int();
		ASSERT_LOG(iv.size() == 2, "XY attribute must be 2 integer elements.");
		true_x_ = x_ = iv[0];
		true_y_ = y_ = iv[1];
	}
	zorder_ = v["zorder"].as_int(0);
	if(v.has_key("on_process")) {
		on_process_ = boost::bind(&widget::process_delegate, this);
		ffl_on_process_ = get_environment()->create_formula(v["on_process"]);
	}
	if(v.has_key("tooltip")) {
		set_tooltip(v["tooltip"].as_string());
	}
	visible_ = v["visible"].as_bool(true);
	if(v.has_key("align_h")) {
		std::string align = v["align_h"].as_string();
		if(align == "left") {
			align_h_ = HALIGN_LEFT;
		} else if(align == "middle" || align == "center" || align == "centre") {
			align_h_ = HALIGN_CENTER;
		} else if(align == "right") {
			align_h_ = HALIGN_RIGHT;
		} else {
			ASSERT_LOG(false, "Invalid align_h attribute given: " << align);
		}
	}
	if(v.has_key("align_v")) {
		std::string align = v["align_v"].as_string();
		if(align == "top") {
			align_v_ = VALIGN_TOP;
		} else if(align == "middle" || align == "center" || align == "centre") {
			align_v_ = VALIGN_CENTER;
		} else if(align == "bottom") {
			align_v_ = VALIGN_BOTTOM;
		} else {
			ASSERT_LOG(false, "Invalid align_v attribute given: " << align);
		}
	}
	disabled_ = !v["enabled"].as_bool(true);
	recalc_loc();
}

widget::~widget()
{
	if(tooltip_displayed_) {
		gui::remove_tooltip(tooltip_);
	}
}

void widget::recalc_loc()
{
	if( align_h_ == HALIGN_LEFT) {
		x_ = true_x_;
	} else if(align_h_ == HALIGN_CENTER) {
		x_ = true_x_ - w_/2;
	} else {
		x_ = true_x_ - w_;
	}

	if( align_v_ == VALIGN_TOP) {
		y_ = true_y_;
	} else if(align_v_ == VALIGN_CENTER) {
		y_ = true_y_ - h_/2;
	} else {
		y_ = true_y_ - h_;
	}
}

void widget::process_delegate()
{
	if(get_environment()) {
		variant value = ffl_on_process_->execute(*get_environment());
		get_environment()->execute_command(value);
	} else {
		std::cerr << "widget::process_delegate() called without environment!" << std::endl;
	}
}

void widget::handle_process()
{
	if(on_process_) {
		on_process_();
	}
}

void widget::process() {
	handle_process();
}

void widget::normalize_event(SDL_Event* event, bool translate_coords)
{
	int tx, ty; //temp x, y
	switch(event->type) {
	case SDL_MOUSEMOTION:
#if TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE
		event->motion.x = (event->motion.x*graphics::screen_width())/preferences::virtual_screen_width();
		event->motion.y = (event->motion.y*graphics::screen_height())/preferences::virtual_screen_height();
#else
		event->motion.x = (event->motion.x*preferences::virtual_screen_width())/preferences::actual_screen_width();
		event->motion.y = (event->motion.y*preferences::virtual_screen_height())/preferences::actual_screen_height();
#endif
		tx = event->motion.x; ty = event->motion.y;
		translate_mouse_coords(&tx, &ty);
		event->motion.x = tx-x();
		event->motion.y = ty-y();
		break;
	case SDL_MOUSEBUTTONDOWN:
	case SDL_MOUSEBUTTONUP:
#if TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE
		event->button.x = (event->button.x*graphics::screen_width())/preferences::virtual_screen_width();
		event->button.y = (event->button.y*graphics::screen_height())/preferences::virtual_screen_height();
#else
		event->button.x = (event->button.x*preferences::virtual_screen_width())/preferences::actual_screen_width();
		event->button.y = (event->button.y*preferences::virtual_screen_height())/preferences::actual_screen_height();
#endif
		tx = event->button.x; ty = event->button.y;
		translate_mouse_coords(&tx, &ty);
		event->button.x = tx-x();
		event->button.y = ty-y();
		break;
	default:
		break;
	}
}

void widget::set_tooltip(const std::string& str)
{
	if(tooltip_displayed_) {
		if(*tooltip_ == str) {
			return;
		}
		gui::remove_tooltip(tooltip_);
		tooltip_displayed_ = false;
	}
	tooltip_.reset(new std::string(i18n::tr(str)));
}

bool widget::process_event(const SDL_Event& event, bool claimed)
{
	if(disabled_) {
		return claimed;
	}
	if(!claimed) {
		if(tooltip_ && event.type == SDL_MOUSEMOTION) {
			if(event.motion.x >= x() && event.motion.x <= x()+width() &&
				event.motion.y >= y() && event.motion.y <= y()+height()) {
				if(!tooltip_displayed_) {
					gui::set_tooltip(tooltip_);
					tooltip_displayed_ = true;
				}
			} else {
				if(tooltip_displayed_) {
					gui::remove_tooltip(tooltip_);
					tooltip_displayed_ = false;
				}
			}
		}
	}

	return handle_event(event, claimed);
}

void widget::draw() const
{
	if(visible_) {
		GLint src = 0;
		GLint dst = 0;
		if(disabled_) {
			glGetIntegerv(GL_BLEND_SRC, &src);
			glGetIntegerv(GL_BLEND_DST, &dst);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glColor4ub(255, 255, 255, disabled_opacity_);
		}
		handle_draw();
		if(disabled_) {
			glBlendFunc(src, dst);
			glColor4ub(255, 255, 255, 255);
		}
	}
}

int widget::x() const
{
	return x_;
}

int widget::y() const
{
	return y_;
}

int widget::width() const
{
	return w_;
}

int widget::height() const
{
	return h_;
}

widget_ptr widget::get_widget_by_id(const std::string& id)
{
	if(id_ == id) {
		return widget_ptr(this);
	}
	return widget_ptr();
}

variant widget::get_value(const std::string& key) const
{
	if(key == "draw_area") {
		std::vector<variant> v;
		v.push_back(variant(x_));
		v.push_back(variant(y_));
		v.push_back(variant(w_));
		v.push_back(variant(h_));
		return variant(&v);
	} else if(key == "tooltip") {
		if(tooltip_) {
			return variant(*tooltip_);
		}
	} else if(key == "is_visible") {
		return variant(visible_);
	} else if(key == "id") {
		return variant(id_);
	}
	return variant();
}

void widget::set_value(const std::string& key, const variant& v)
{
	if(key == "width") {
		w_ = v.as_int();
	} else if(key == "height") {
		h_ = v.as_int();
	} else if(key == "rect" || key == "draw_area") {
		std::vector<int> r = v.as_list_int();
		ASSERT_LOG(r.size() == 4, "Four values must be supplied to the rect attribute");
		set_loc(r[0], r[1]);
		set_dim(r[2], r[3]);
	} else if(key == "xy" || key == "left_top") {
		std::vector<int> xy = v.as_list_int();
		ASSERT_LOG(xy.size() == 2, "Two values must be supplied to the X, Y attribute");
		set_loc(xy[0], xy[1]);
	} else if(key == "wh") {
		std::vector<int> wh = v.as_list_int();
		ASSERT_LOG(wh.size() == 2, "Two values must be supplied to the W, H attribute");
		set_dim(wh[0], wh[1]);
	} else if(key == "right_bottom") {
		std::vector<int> rb = v.as_list_int();
		ASSERT_LOG(rb.size() == 2, "Two values must be supplied to the R, B attribute");
		set_dim(rb[0] - x(), rb[1] - y());
	} else if(key == "left") {
		x_ = v.as_int();
	} else if(key == "top") {
		y_ = v.as_int();
	} else if(key == "right") {
		w_ = v.as_int() - x();
	} else if(key == "bottom") {
		h_ = v.as_int() - y();
	} else if(key == "visible") {
		visible_ = v.as_bool();
	} else if(key == "id") {
		id_ = v.as_string();
	} else if(key == "disable") {
		disabled_ = v.as_bool();
	} else if(key == "enable") {
		disabled_ = !v.as_bool();
	} else if(key == "disabled_opacity") {
		int opa = v.as_int();
		disabled_opacity_ = (opa > 255) ? 255 : (opa < 0) ? 0 : opa;
	}
}

}
