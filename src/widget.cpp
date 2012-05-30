
/*
   Copyright (C) 2007 by David White <dave@whitevine.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

#include "asserts.hpp"
#include "preferences.hpp"
#include "raster.hpp"
#include "tooltip.hpp"
#include "i18n.hpp"
#include "widget.hpp"
#include "iphone_controls.hpp"

#include <iostream>

namespace gui {

widget::widget(const variant& v, const game_logic::formula_callable_ptr& e) 
	: environ_(e)
{
	if(v.has_key("width")) {
		w_ = v["width"].as_int();
	} else if(v.has_key("height")) {
		h_ = v["height"].as_int();
	} else if(v.has_key("rect")) {
		std::vector<int> r = v["rect"].as_list_int();
		ASSERT_LOG(r.size() == 4, "Four values must be supplied to the rect attribute");
		set_loc(r[0], r[1]);
		set_dim(r[2], r[3]);
	} else if(v.has_key("draw_area")) {
		std::vector<int> r = v["draw_area"].as_list_int();
		ASSERT_LOG(r.size() == 4, "Four values must be supplied to the rect attribute");
		set_loc(r[0], r[1]);
		set_dim(r[2], r[3]);
	} else if(v.has_key("x")) {
		x_ = v["x"].as_int();
	} else if(v.has_key("y")) {
		y_ = v["y"].as_int();
	}
}

widget::~widget()
{
	if(tooltip_displayed_) {
		gui::remove_tooltip(tooltip_);
	}
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
		handle_draw();
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
	}
}

}
