
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

#include <iostream>

#include "dialog.hpp"
#include "font.hpp"
#include "foreach.hpp"
#include "raster.hpp"
#include "surface_cache.hpp"
#include "texture.hpp"
#include "tooltip.hpp"
#include "draw_scene.hpp"
#include "level.hpp"
#include "framed_gui_element.hpp"
#include "preferences.hpp"

namespace gui {

dialog::dialog(int x, int y, int w, int h)
  : opened_(false), cancelled_(false), clear_bg_(196), padding_(10),
    add_x_(0), add_y_(0), bg_alpha_(1.0), last_draw_(-1)
{
	set_loc(x,y);
	set_dim(w,h);
}

dialog::~dialog()
{}

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
	return *this;
}

void dialog::remove_widget(widget_ptr w)
{
    deregister_listener(w);
	widgets_.erase(std::remove(widgets_.begin(),widgets_.end(),w),
	               widgets_.end());
}

void dialog::clear() { 
    foreach(widget_ptr w, widgets_) {
        deregister_listener(w);
    }
    widgets_.clear(); 
}

void dialog::replace_widget(widget_ptr w_old, widget_ptr w_new)
{
	int x = w_old->x();
	int y = w_old->y();
	int w = w_old->width();
	int h = w_old->height();

	std::replace(widgets_.begin(), widgets_.end(), w_old, w_new);

	w_new->set_loc(x,y);
	w_new->set_dim(w,h);

    register_listener(w_new);
    deregister_listener(w_old);
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
	SDL_GL_SwapBuffers();
#if defined(__ANDROID__)
	graphics::reset_opengl_state();
#endif

	const int end_draw = last_draw_ + 20;
	const int delay_time = std::max<int>(1, end_draw - SDL_GetTicks());

	SDL_Delay(delay_time);

	last_draw_ = SDL_GetTicks();
}

void dialog::handle_draw_children() const {
	glPushMatrix();
	glTranslatef(x(),y(),0.0);
	foreach(const widget_ptr& w, widgets_) {
		w->draw();
	}
	glPopMatrix();
}

void dialog::handle_draw() const
{
	if(clear_bg()) {
		SDL_Rect rect = {x(),y(),width(),height()};
		SDL_Color col = {0,0,0,0};
		graphics::draw_rect(rect,col,clear_bg_);

		//fade effect for fullscreen dialogs
		if(bg_.valid()) {
			if(bg_alpha_ > 0.25) {
				bg_alpha_ -= 0.05;
			}
			glColor4f(1.0, 1.0, 1.0, bg_alpha_);
			graphics::blit_texture(bg_, x(), y(), width(), height(), 0.0, 0.0, 1.0, 1.0, 0.0);
			glColor4f(1.0, 1.0, 1.0, 1.0);
		}
	}
	if (level::current_ptr() && !level::current().in_editor())
	{
		draw_scene(level::current(), last_draw_position());
		SDL_Rect rect = {x(),y(),width(),height()};
		SDL_Color col = {0,0,0,0};
		graphics::draw_rect(rect, col, 204);
		const_framed_gui_element_ptr window(framed_gui_element::get("empty_window"));
		window->blit(x(),y(),width(),height(), 1);
	}
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

bool dialog::handle_event(const SDL_Event& event, bool claimed)
{

    claimed |= handle_event_children(event, claimed);

    if(!claimed && opened_) {
        if(event.type == SDL_KEYDOWN &&
           event.key.keysym.sym == SDLK_RETURN) {
            close();
			cancelled_ = false;
            claimed = true;
        } else if(event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
			close();
			cancelled_ = true;
			claimed = true;
		}
    }

	if(!claimed) {
		switch(event.type) {
		//if it's a mouse button up or down and it's within the dialog area,
		//then we claim it because nobody else should get it.
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP: {
			int mousex, mousey;
			SDL_GetMouseState(&mousex, &mousey);
			if(mousex >= x() && mousex < x() + width() &&
			   mousey >= y() && mousey < y() + height()) {
				claimed = true;
			}
			break;
		}
		}
	}
	return claimed;
}


}
