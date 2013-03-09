
/*
   Copyright (C) 2007 by David White <dave@whitevine.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#include "graphics.hpp"

#include <boost/bind.hpp>
#include <iostream>

#include "dialog.hpp"
#include "font.hpp"
#include "foreach.hpp"
#include "module.hpp"
#include "raster.hpp"
#include "surface_cache.hpp"
#include "texture.hpp"
#include "tooltip.hpp"
#include "draw_scene.hpp"
#include "level.hpp"
#include "framed_gui_element.hpp"
#include "preferences.hpp"
#include "widget_factory.hpp"

namespace gui {

namespace {

module::module_file_map& get_dialog_path()
{
	static module::module_file_map dialog_file_map;
	return dialog_file_map;
}

void load_dialog_file_paths(const std::string& path)
{
	if(get_dialog_path().empty()) {
		module::get_unique_filenames_under_dir(path, &get_dialog_path());
	}
}

}

void reset_dialog_paths()
{
	get_dialog_path().clear();
}

std::string get_dialog_file(const std::string& fname)
{
	load_dialog_file_paths("data/dialog/");
	module::module_file_map::const_iterator it = module::find(get_dialog_path(), fname);
	ASSERT_LOG(it != get_dialog_path().end(), "DIALOG FILE NOT FOUND: " << fname);
	return it->second;
}

dialog::dialog(int x, int y, int w, int h)
  : opened_(false), cancelled_(false), clear_bg_(196), padding_(10),
    add_x_(0), add_y_(0), bg_alpha_(1.0), last_draw_(-1), upscale_frame_(true)
{
	set_environment();
	set_loc(x,y);
	set_dim(w,h);
	forced_dimensions_ = rect(x, y, w, h);
}

dialog::dialog(const variant& v, game_logic::formula_callable* e)
	: widget(v,e),
	opened_(false), cancelled_(false), 
	add_x_(0), add_y_(0), last_draw_(-1),
	upscale_frame_(v["upscale_frame"].as_bool(true))
{
	forced_dimensions_ = rect(x(), y(), width(), height());
	padding_ = v["padding"].as_int(10);
	if(v.has_key("background_frame")) {
		background_framed_gui_element_ = v["background_frame"].as_string();
	}
	if(v.has_key("background_draw")) {
		std::string scene = v["background_draw"].as_string();
		if(scene == "last_scene") {
			draw_background_fn_ = boost::bind(&dialog::draw_last_scene);
		}
		// XXX could make this FFL callable. Or could allow any of the background scenes to be drawn. or both.
	}
	clear_bg_ = v["clear_background_alpha"].as_int(196);
	bg_alpha_ = v["background_alpha"].as_int(255) / GLfloat(255.0);
	if(v.has_key("cursor")) {
		std::vector<int> vi = v["cursor"].as_list_int();
		set_cursor(vi[0], vi[1]);
	}
	if(v.has_key("on_quit")) {
		on_quit_ = boost::bind(&dialog::quit_delegate, this);
		ASSERT_LOG(get_environment() != NULL, "environment not set");
		const variant on_quit_value = v["on_quit"];
		if(on_quit_value.is_function()) {
			ASSERT_LOG(on_quit_value.min_function_arguments() == 0, "on_quit_value dialog function should take 0 arguments: " << v.debug_location());
			static const variant fml("fn()");
			ffl_on_quit_.reset(new game_logic::formula(fml));

			game_logic::map_formula_callable* callable = new game_logic::map_formula_callable;
			callable->add("fn", on_quit_value);

			quit_arg_.reset(callable);
		} else {
			ffl_on_quit_ = get_environment()->create_formula(v["on_quit"]);
		}
	}
	if(v.has_key("on_close")) {
		on_close_ = boost::bind(&dialog::close_delegate, this, _1);
		ASSERT_LOG(get_environment() != NULL, "environment not set");
		const variant on_close_value = v["on_close"];
		if(on_close_value.is_function()) {
			ASSERT_LOG(on_close_value.min_function_arguments() <= 1 && on_close_value.max_function_arguments() >= 1, "on_close dialog function should take 1 argument: " << v.debug_location());
			static const variant fml("fn(selection)");
			ffl_on_close_.reset(new game_logic::formula(fml));

			game_logic::map_formula_callable* callable = new game_logic::map_formula_callable;
			callable->add("fn", on_close_value);

			close_arg_.reset(callable);
		} else {
			ffl_on_close_ = get_environment()->create_formula(v["on_close"]);
		}
	}
	std::vector<variant> children = v["children"].as_list();
	foreach(const variant& child, children) {
		widget_ptr w = widget_factory::create(child, e);
		if(w->x() != 0 || w->y() != 0) {
		//if(child.has_key("add_xy")) {
		//	std::vector<int> addxy = child["add_xy"].as_list_int();
		//	add_widget(widget_factory::create(child, e), addxy[0], addxy[1]);
			add_widget(w, w->x(), w->y());
		} else {
			add_widget(w);
		}
	}
	recalculate_dimensions();
}

dialog::~dialog()
{}

void dialog::recalculate_dimensions()
{
	if(forced_dimensions_.empty()) {
		int new_w = 0;
		int new_h = 0;
	    foreach(widget_ptr w, widgets_) {
			if((w->x() + w->width()) > new_w) {
				new_w = w->x() + w->width() + padding_ + get_pad_width();
			}
			if((w->y() + w->height()) > new_h) {
				new_h = w->y() + w->height() + padding_ + get_pad_height();
			}
		}
		set_dim(new_w, new_h);
	}
}

void dialog::draw_last_scene() 
{
	draw_scene(level::current(), last_draw_position());
}

void dialog::quit_delegate()
{
	if(quit_arg_) {
		using namespace game_logic;
		variant value = ffl_on_quit_->execute(*quit_arg_);
		get_environment()->execute_command(value);
	} else if(get_environment()) {
		variant value = ffl_on_quit_->execute(*get_environment());
		get_environment()->execute_command(value);
	} else {
		std::cerr << "dialog::quit_delegate() called without environment!" << std::endl;
	}
}

void dialog::close_delegate(bool cancelled)
{
	using namespace game_logic;
	if(close_arg_) {
		using namespace game_logic;
		map_formula_callable_ptr callable = map_formula_callable_ptr(new map_formula_callable(close_arg_.get()));
		callable->add("cancelled", variant::from_bool(cancelled));
		variant value = ffl_on_close_->execute(*callable);
		get_environment()->execute_command(value);
	} else if(get_environment()) {
		map_formula_callable_ptr callable = map_formula_callable_ptr(new map_formula_callable(get_environment()));
		callable->add("cancelled", variant::from_bool(cancelled));
		variant value = ffl_on_close_->execute(*callable);
		get_environment()->execute_command(value);
	} else {
		std::cerr << "dialog::close_delegate() called without environment!" << std::endl;
	}
}

void dialog::handle_process()
{
	widget::handle_process();
    foreach(widget_ptr w, widgets_) {
		w->process();
	}
}

dialog& dialog::add_widget(widget_ptr w, dialog::MOVE_DIRECTION dir)
{
	add_widget(w, add_x_, add_y_, dir);
	return *this;
}

dialog& dialog::add_widget(widget_ptr w, int x, int y,
                           dialog::MOVE_DIRECTION dir)
{
	w->set_loc(x,y);
	widgets_.push_back(w);
	std::stable_sort(widgets_.begin(), widgets_.end(), widget_sort_zorder());
    register_listener(w);
	switch(dir) {
	case MOVE_DOWN:
		add_x_ = x;
		add_y_ = y + w->height() + padding_;
		break;
	case MOVE_RIGHT:
		add_x_ = x + w->width() + padding_;
		add_y_ = y;
		break;
	}
	recalculate_dimensions();
	return *this;
}

void dialog::remove_widget(widget_ptr w)
{
    deregister_listener(w);
	widgets_.erase(std::remove(widgets_.begin(),widgets_.end(),w),
	               widgets_.end());
	recalculate_dimensions();
}

void dialog::clear() { 
    foreach(widget_ptr w, widgets_) {
        deregister_listener(w);
    }
    widgets_.clear(); 
	recalculate_dimensions();
}

void dialog::replace_widget(widget_ptr w_old, widget_ptr w_new)
{
	int x = w_old->x();
	int y = w_old->y();
	int w = w_old->width();
	int h = w_old->height();

	std::replace(widgets_.begin(), widgets_.end(), w_old, w_new);
	std::stable_sort(widgets_.begin(), widgets_.end(), widget_sort_zorder());

	w_new->set_loc(x,y);
	w_new->set_dim(w,h);

    register_listener(w_new);
    deregister_listener(w_old);
	recalculate_dimensions();
}

void dialog::show() {
	opened_ = true;
	set_visible(true);
}

void dialog::show_modal()
{
    input::pump pump;
	opened_ = true;
	cancelled_ = false;
    pump.register_listener(this);

	while(opened_ && pump.process()) {
		if(on_process_) {
			on_process_();
		}
		prepare_draw();
		draw();
		gui::draw_tooltip();
		complete_draw();
	}
}

void dialog::prepare_draw()
{
	graphics::prepare_raster();
	if(clear_bg()) {
		glClearColor(0.0,0.0,0.0,0.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}
}

void dialog::complete_draw()
{
	graphics::swap_buffers();

	const int end_draw = last_draw_ + 20;
	const int delay_time = std::max<int>(1, end_draw - SDL_GetTicks());

	SDL_Delay(delay_time);

	last_draw_ = SDL_GetTicks();
}

void dialog::handle_draw_children() const {
	glPushMatrix();
	glTranslatef(GLfloat(x()),GLfloat(y()),0.0);
	foreach(const widget_ptr& w, widgets_) {
		w->draw();
	}
	glPopMatrix();
}

void dialog::handle_draw() const
{
	GLfloat current_color[4];
#if defined(USE_GLES2)
	memcpy(current_color, gles2::get_color(), sizeof(current_color));
#else
	glGetFloatv(GL_CURRENT_COLOR, current_color);
#endif

	if(clear_bg()) {
		SDL_Rect rect = {x(),y(),width(),height()};
		SDL_Color col = {0,0,0,0};
		graphics::draw_rect(rect,col,clear_bg_);

		//fade effect for fullscreen dialogs
		if(bg_.valid()) {
			if(bg_alpha_ > 0.25) {
				bg_alpha_ -= GLfloat(0.05);
			}
			glColor4f(1.0, 1.0, 1.0, bg_alpha_);
			graphics::blit_texture(bg_, x(), y(), width(), height(), 0.0, 0.0, 1.0, 1.0, 0.0);
			glColor4f(1.0, 1.0, 1.0, 1.0);
		}
	}

	if(draw_background_fn_) {
		draw_background_fn_();
	}

	if(background_framed_gui_element_.empty() == false) {
		SDL_Rect rect = {x(),y(),width(),height()};
		SDL_Color col = {0,0,0,0};
		graphics::draw_rect(rect, col, get_alpha() >= 255 ? 204 : get_alpha());
		const_framed_gui_element_ptr window(framed_gui_element::get(background_framed_gui_element_));
		window->blit(x(),y(),width(),height(), upscale_frame_);
	}

	glColor4f(current_color[0], current_color[1], current_color[2], current_color[3]);
	handle_draw_children();
}

bool dialog::process_event(const SDL_Event& ev, bool claimed) {
	if (ev.type == SDL_QUIT && on_quit_)
		on_quit_();
    return widget::process_event(ev, claimed);
}

bool dialog::handle_event_children(const SDL_Event &event, bool claimed) {
	SDL_Event ev = event;
	normalize_event(&ev, false);
    return input::listener_container::process_event(ev, claimed);
}

void dialog::close() 
{ 
	opened_ = false; 
	if(on_close_) {
		on_close_(cancelled_);
	}
}


bool dialog::handle_event(const SDL_Event& ev, bool claimed)
{

    claimed |= handle_event_children(ev, claimed);

    if(!claimed && opened_) {
        if(ev.type == SDL_KEYDOWN &&
           ev.key.keysym.sym == SDLK_RETURN) {
            close();
			cancelled_ = false;
            claimed = true;
        } else if(ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_ESCAPE) {
			close();
			cancelled_ = true;
			claimed = true;
		}
    }

	if(!claimed) {
		switch(ev.type) {
		//if it's a mouse button up or down and it's within the dialog area,
		//then we claim it because nobody else should get it.
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP: {
			if(claim_mouse_events() && in_widget(ev.button.x, ev.button.y)) {
				claimed = true;
			}
			break;
		}
		}
	}
	return claimed;
}

bool dialog::has_focus() const
{
	foreach(widget_ptr w, widgets_) {
		if(w->has_focus()) {
			return true;
		}
	}

	return false;
}

widget_ptr dialog::get_widget_by_id(const std::string& id)
{
	foreach(widget_ptr w, widgets_) {
		if(w) {
			widget_ptr wx = w->get_widget_by_id(id);
			if(wx) {
				return wx;
			}
		}
	}
	return widget::get_widget_by_id(id);
}

const_widget_ptr dialog::get_widget_by_id(const std::string& id) const
{
	foreach(widget_ptr w, widgets_) {
		if(w) {
			widget_ptr wx = w->get_widget_by_id(id);
			if(wx) {
				return wx;
			}
		}
	}
	return widget::get_widget_by_id(id);
}

void dialog::set_value(const std::string& key, const variant& v)
{
	if(key == "child") {
		widget_ptr w = widget_factory::create(v, get_environment());
		add_widget(w, w->x(), w->y());
	}
	widget::set_value(key, v);
}

variant dialog::get_value(const std::string& key) const
{
	if(key == "children") {
		std::vector<variant> v;
	    foreach(widget_ptr w, widgets_) {
			v.push_back(variant(w.get()));
		}
		return variant(&v);
	}
	return widget::get_value(key);
}

}
