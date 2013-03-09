
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
#include "tooltip.hpp"
#include "framed_gui_element.hpp"

namespace gui {

class widget;

typedef boost::intrusive_ptr<widget> widget_ptr;
typedef boost::intrusive_ptr<const widget> const_widget_ptr;

struct color_save_context
{
	color_save_context()
	{
#if defined(USE_GLES2)
		memcpy(current_color, gles2::get_color(), sizeof(current_color));
#else
		glGetFloatv(GL_CURRENT_COLOR, current_color);
#endif
	}
	~color_save_context()
	{
		glColor4f(current_color[0], current_color[1], current_color[2], current_color[3]);
	}
	GLfloat current_color[4];
};

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
	void set_tooltip(const std::string& str, int fontsize=18, const SDL_Color& color=graphics::color_yellow(), const std::string& font="");
	bool visible() { return visible_; }
	void set_visible(bool visible) { visible_ = visible; }
	std::string id() const { return id_; }
	bool disabled() const { return disabled_; }
	void enable(bool val=true) { disabled_ = val; }
	bool claim_mouse_events() const { return claim_mouse_events_; }
	void set_claim_mouse_events(bool claim=true) { claim_mouse_events_ = claim; }

	unsigned get_tooltip_delay() const { return tooltip_display_delay_; }
	void set_tooltip_delay(unsigned tooltip_delay) { tooltip_display_delay_ = tooltip_delay; }

	virtual void set_value(const std::string& key, const variant& v);
	virtual variant get_value(const std::string& key) const;

	virtual widget_ptr get_widget_by_id(const std::string& id);
	virtual const_widget_ptr get_widget_by_id(const std::string& id) const;

	virtual void handle_draw() const = 0;

	virtual bool has_focus() const { return false; }
	game_logic::formula_callable* get_environment() const { return environ_; }

	void set_zorder(int z) { zorder_ = z; }
	int zorder() const { return zorder_; }

	int get_frame_resolution() const { return resolution_; }
	void set_frame_set(const std::string& frame) { frame_set_ = framed_gui_element::get(frame); frame_set_name_ = frame; }

	int get_alpha() const { return display_alpha_; }
	void set_alpha(int a=256) { display_alpha_ = a; }

	int get_pad_width() const { return pad_w_; }
	int get_pad_height() const { return pad_h_; }
	void set_padding(int pw, int ph) { pad_w_ = pw; pad_h_ = ph; }

	void process();
protected:
	widget();
	explicit widget(const variant& v, game_logic::formula_callable* e);
	virtual ~widget();

	void normalize_event(SDL_Event* event, bool translate_coords=false);
	virtual bool handle_event(const SDL_Event& event, bool claimed) { return claimed; }
	void set_environment(game_logic::formula_callable* e = 0) { environ_ = e; }
	boost::function<void()> on_process_;
	virtual void handle_process();
	virtual void recalc_loc();
	virtual bool in_widget(int xloc, int yloc) const;
private:
	int x_, y_;
	int w_, h_;
	int true_x_;
	int true_y_;
	boost::shared_ptr<gui::tooltip_item> tooltip_;
	bool tooltip_displayed_;
	int tooltip_fontsize_;
	SDL_Color tooltip_color_;
	std::string tooltip_font_;
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
	unsigned tooltip_display_delay_;
	unsigned tooltip_ticks_;
	int display_alpha_;
	int pad_h_;
	int pad_w_;
	bool claim_mouse_events_;

	HORIZONTAL_ALIGN align_h_;
	VERTICAL_ALIGN   align_v_;

	std::string frame_set_name_;
	const_framed_gui_element_ptr frame_set_;
	int resolution_;
};

// Functor to sort widgets by z-ordering.
class widget_sort_zorder
{
public:
    bool operator()(const widget_ptr lhs, const widget_ptr rhs) const;
};

}

#endif
