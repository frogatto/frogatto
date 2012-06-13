
/*
   Copyright (C) 2007 by David White <dave@whitevine.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef DIALOG_HPP_INCLUDED
#define DIALOG_HPP_INCLUDED

#include "texture.hpp"
#include "widget.hpp"

#include <string>
#include <vector>

namespace gui {

std::string get_dialog_file(const std::string& fname);
void reset_dialog_paths();

class dialog : public virtual widget, public virtual input::listener_container
{
public:
	typedef std::vector<widget_ptr>::const_iterator child_iterator;

	explicit dialog(int x, int y, int w, int h);
	explicit dialog(const variant& v, game_logic::formula_callable* e);
	virtual ~dialog();
	virtual void show_modal();
	void show();

	enum MOVE_DIRECTION { MOVE_DOWN, MOVE_RIGHT };
	dialog& add_widget(widget_ptr w, MOVE_DIRECTION dir=MOVE_DOWN);
	dialog& add_widget(widget_ptr w, int x, int y,
	                MOVE_DIRECTION dir=MOVE_DOWN);
	void remove_widget(widget_ptr w);
	void replace_widget(widget_ptr w_old, widget_ptr w_new);
	void clear();
	int padding() const { return padding_; }
	void set_padding(int pad) { padding_ = pad; }
	void close() { opened_ = false; }
	void cancel() { cancelled_ = true; close(); }

	bool closed() { return !opened_; }
	bool cancelled() { return cancelled_; }
	void set_cursor(int x, int y) { add_x_ = x; add_y_ = y; }
	int cursor_x() const { return add_x_; }
	int cursor_y() const { return add_y_; }
	child_iterator begin_children() const { return widgets_.begin(); }
	child_iterator end_children() const { return widgets_.end(); }
    bool process_event(const SDL_Event& e, bool claimed);
	
	void set_on_quit(boost::function<void ()> onquit) { on_quit_ = onquit; }

	void set_background_frame(const std::string& id) { background_framed_gui_element_ = id; }
	void set_draw_background_fn(boost::function<void()> fn) { draw_background_fn_ = fn; }

	virtual bool has_focus() const;
	void set_process_hook(boost::function<void()> fn) { on_process_ = fn; }
	static void draw_last_scene();
protected:
	virtual bool handle_event(const SDL_Event& event, bool claimed);
	virtual bool handle_event_children(const SDL_Event& event, bool claimed);
	virtual void handle_draw() const;
	virtual void handle_draw_children() const;
	void prepare_draw();
	void complete_draw();
	void set_clear_bg(bool clear) { clear_bg_ = clear; };
	void set_clear_bg_amount(int amount) { clear_bg_ = amount; }
	int clear_bg() const { return clear_bg_; };

	virtual void set_value(const std::string& key, const variant& v);
	virtual variant get_value(const std::string& key) const;

	virtual void handle_process();
private:
	std::vector<widget_ptr> widgets_;
	bool opened_;
	bool cancelled_;
	int clear_bg_;
	
	boost::function<void ()> on_quit_;

	void quit_delegate();

	game_logic::formula_ptr ffl_on_quit_;

	//default padding between widgets
	int padding_;

	//where the next widget will be placed by default
	int add_x_, add_y_;

	graphics::texture bg_;
	mutable GLfloat bg_alpha_;

	int last_draw_;

	std::string background_framed_gui_element_;
	boost::function<void()> draw_background_fn_;
};

typedef boost::intrusive_ptr<dialog> dialog_ptr;

}

#endif
