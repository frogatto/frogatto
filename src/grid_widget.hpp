
/*
   Copyright (C) 2007 by David White <dave@whitevine.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef GRID_WIDGET_HPP_INCLUDED
#define GRID_WIDGET_HPP_INCLUDED

#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <vector>

#include "grid_widget_fwd.hpp"
#include "scrollable_widget.hpp"
#include "widget.hpp"

namespace gui {

class grid : public scrollable_widget
{
public:
	typedef boost::function<void (int)> callback_type;
	enum COLUMN_ALIGN { ALIGN_LEFT, ALIGN_CENTER, ALIGN_RIGHT };

	explicit grid(int ncols);
	virtual ~grid() {}
	grid& set_show_background(bool val) {
		show_background_ = val;
		return *this;
	}
	void add_row(const std::vector<widget_ptr>& widgets);

	grid& add_col(const widget_ptr& widget=widget_ptr());

	grid& finish_row();

	grid& set_col_width(int col, int width);
	grid& set_align(int col, COLUMN_ALIGN align);
	grid& set_hpad(int pad);

	void allow_selection(bool val=true) { allow_selection_ = val; }
	void must_select(bool val=true, int nrow=0) { must_select_ = val; selected_row_ = nrow; }
	bool has_must_select() const { return must_select_; }
	void swallow_clicks(bool val=true) { swallow_clicks_ = val; }
	int selection() const { return selected_row_; }
	void register_mouseover_callback(callback_type cb);
	void register_selection_callback(callback_type cb);
	void register_row_selection_callback(boost::function<void()> cb);

	void set_max_height(int amount) { max_height_ = amount; }

	void on_set_yscroll(int old_value, int value);
private:
	int row_at(int x, int y) const;
	void recalculate_dimensions();
	void handle_draw() const;
	bool handle_event(const SDL_Event& event, bool claimed);

	int nrows() const { return cells_.size()/ncols_; }
	int ncols_;
	std::vector<widget_ptr> cells_;
	std::vector<widget_ptr> visible_cells_;
	std::vector<int> col_widths_;
	std::vector<COLUMN_ALIGN> col_aligns_;
	int row_height_;
	int selected_row_;
	bool allow_selection_;
	bool must_select_;
	bool swallow_clicks_;

	std::vector<widget_ptr> new_row_;
	std::vector<boost::function<void()> > row_callbacks_;
	callback_type on_mouseover_;
	callback_type on_select_;
	int hpad_;
	bool show_background_;

	int max_height_;
};

typedef boost::shared_ptr<grid> grid_ptr;
typedef boost::shared_ptr<const grid> const_grid_ptr;

}

#endif
