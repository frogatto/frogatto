
/*
   Copyright (C) 2007 by David White <dave@whitevine.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef WIDGET_HPP_INCLUDED
#define WIDGET_HPP_INCLUDED

#include <string>
#include <boost/function.hpp>

#include "formula.hpp"
#include "graphics.hpp"
#include "input.hpp"

namespace gui {

class widget;

typedef boost::intrusive_ptr<widget> widget_ptr;
typedef boost::intrusive_ptr<const widget> const_widget_ptr;

class widget : public virtual input::listener
{
public:
	enum HORIZONTAL_ALIGN {HALIGN_LEFT, HALIGN_CENTER, HALIGN_RIGHT};
	enum VERTICAL_ALIGN   {VALIGN_TOP,  VALIGN_CENTER, VALIGN_BOTTOM};

	bool process_event(const SDL_Event& event, bool claimed);
	void draw() const;

	virtual void set_loc(int x, int y) { true_x_ = x_ = x; true_y_ = y_ = y; recalc_loc(); }
	virtual void set_dim(int w, int h) { w_ = w; h_ = h; recalc_loc(); }

	int x() const;
	int y() const;
	int width() const;
	int height() const;
	void set_tooltip(const std::string& str);
	bool visible() { return visible_; }
	void set_visible(bool visible) { visible_ = visible; }
	std::string id() const { return id_; }
	bool disabled() const { return disabled_; }
	bool enable(bool val=true) { disabled_ = val; }

	virtual void set_value(const std::string& key, const variant& v);
	virtual variant get_value(const std::string& key) const;

	virtual widget_ptr get_widget_by_id(const std::string& id);

	virtual void handle_draw() const = 0;

	virtual bool has_focus() const { return false; }
	game_logic::formula_callable* get_environment() const { return environ_; }

	void set_zorder(int z) { zorder_ = z; }
	int zorder() const { return zorder_; }

	void process();
protected:
	widget() 
		: x_(0), y_(0), w_(0), h_(0), align_h_(HALIGN_LEFT), align_v_(VALIGN_TOP),
		true_x_(0), true_y_(0), disabled_(false), disabled_opacity_(127),
		tooltip_displayed_(false), visible_(true), zorder_(0), environ_(0)
	{}
	explicit widget(const variant& v, game_logic::formula_callable* e);
	virtual ~widget();

	void normalize_event(SDL_Event* event, bool translate_coords=false);
	virtual bool handle_event(const SDL_Event& event, bool claimed) { return claimed; }
	void set_environment(game_logic::formula_callable* e = 0) { environ_ = e; }
	boost::function<void()> on_process_;
	virtual void handle_process();
	virtual void recalc_loc();
private:
	int x_, y_;
	int w_, h_;
	int true_x_;
	int true_y_;
	boost::shared_ptr<std::string> tooltip_;
	bool tooltip_displayed_;
	bool visible_;
	game_logic::formula_callable* environ_;
	void process_delegate();
	game_logic::formula_ptr ffl_on_process_;
	// default zorder_ is 0.  A widget *must* have a good reason for wanting
	// higher priority in the draw order.
	int zorder_;
	std::string id_;
	bool disabled_;
	uint8_t disabled_opacity_;

	HORIZONTAL_ALIGN align_h_;
	VERTICAL_ALIGN   align_v_;
};

// Functor to sort widgets by z-ordering.
class widget_sort_zorder
{
public:
    bool operator()(const widget_ptr lhs, const widget_ptr rhs) const
    {
        return lhs->zorder() < rhs->zorder();
    }
};

}

#endif
