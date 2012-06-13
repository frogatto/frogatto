
/*
   Copyright (C) 2007 by David White <dave@whitevine.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#include <iostream>
#include <boost/bind.hpp>

#include "foreach.hpp"
#include "grid_widget.hpp"
#include "raster.hpp"
#include "widget_factory.hpp"

namespace gui {

grid::grid(int ncols)
  : ncols_(ncols), col_widths_(ncols, 0),
    col_aligns_(ncols, grid::ALIGN_LEFT), row_height_(0),
	selected_row_(-1), allow_selection_(false), must_select_(false),
    swallow_clicks_(false), hpad_(0), show_background_(false),
	max_height_(-1), allow_highlight_(true), set_h_(0), set_w_(0)
{
	set_environment();
	set_dim(0,0);
}

grid::grid(const variant& v, game_logic::formula_callable* e)
	: scrollable_widget(v, e), row_height_(0), selected_row_(-1), 
	allow_selection_(false), must_select_(false),
    swallow_clicks_(false), hpad_(0), show_background_(false),
	max_height_(-1), allow_highlight_(true), set_h_(0), set_w_(0)
{
	ASSERT_LOG(get_environment() != 0, "You must specify a callable environment");
	if(v.has_key("on_select")) {
		ffl_on_select_ = get_environment()->create_formula(v["on_select"]);
		on_select_ = boost::bind(&grid::select_delegate, this, _1);
	}
	if(v.has_key("on_mouseover")) {
		ffl_on_mouseover_ = get_environment()->create_formula(v["on_mouseover"]);
		on_select_ = boost::bind(&grid::mouseover_delegate, this, _1);
	}

	ncols_ = v.as_int(1);
	if(v.has_key("column_widths")) {
		if(v["column_widths"].is_list()) {
			col_widths_.assign(v["column_widths"].as_list_int().begin(), v["column_widths"].as_list_int().end());
		} else if(v["column_widths"].is_int()) {
			col_widths_.assign(ncols_, v["column_widths"].as_int());
		} else {
			ASSERT_LOG(false, "grid: column_widths must be an int or list of ints");
		}
	} else {
		col_widths_.assign(ncols_, 0);
	}
	if(v.has_key("column_alignments")) {
		if(v["column_alignments"].is_list()) {
			// XXX this could be a list of strings as well.
			int col = 0;
			foreach(const variant& c, v["column_alignments"].as_list()) {
				if(c.is_int()) {
					set_align(col, static_cast<COLUMN_ALIGN>(c.as_int()));
				} else if(c.is_string()) {
					const std::string& s = v["column_alignments"].as_string();
					if(s == "center" || s == "centre") {
						set_align(col, ALIGN_CENTER);
					} else if(s == "right") {
						set_align(col, ALIGN_RIGHT);
					} else if(s == "left") {
						set_align(col, ALIGN_LEFT);
					} else {
						ASSERT_LOG(false, "grid: column_alignments must be \"left\", \"right\" or \"center\"");
					}
				} else {
					ASSERT_LOG(false, "grid: column alignment members must be an integer or a string.");
				}
				col++;
			}
		} else if(v["column_alignments"].is_int()) {
			col_aligns_.assign(ncols_, static_cast<COLUMN_ALIGN>(v["column_alignments"].as_int()));
		} else if(v["column_alignments"].is_string()) {
			const std::string& s = v["column_alignments"].as_string();
			if(s == "center" || s == "centre") {
				col_aligns_.assign(ncols_, ALIGN_CENTER);
			} else if(s == "right") {
				col_aligns_.assign(ncols_, ALIGN_RIGHT);
			} else if(s == "left") {
				col_aligns_.assign(ncols_, ALIGN_LEFT);
			} else {
				ASSERT_LOG(false, "grid: column_alignments must be \"left\", \"right\" or \"center\"");
			}
		} else {
			ASSERT_LOG(false, "grid: column_alignments must be an int or list of ints");
		}
	} else {
		col_aligns_.assign(ncols_, ALIGN_LEFT);
	}

	allow_selection_ = v["allow_selection"].as_bool(false);
	if(v.has_key("must_select")) {
		must_select_ = v["must_select_"].as_bool();
		if(v.has_key("must_select_row")) {
			selected_row_ = v["must_select_row"].as_int();
		}
	}
	if(v.has_key("swallow_clicks")) {
		swallow_clicks_ = v["swallow_clicks"].as_bool();
	}
	if(v.has_key("max_height")) {
		max_height_ = v["max_height"].as_int();
	}
	if(v.has_key("allow_draw_highlight")) {
		allow_highlight_ = v["allow_draw_highlight"].as_bool();
	}
	if(v.has_key("header_rows")) {
		if(v["header_rows"].is_int()) {
			set_header_row(v["header_rows"].as_int());
		} else if(v["header_rows"].is_list()) {
			header_rows_.assign(v["header_rows"].as_list_int().begin(), v["header_rows"].as_list_int().end());
		} else {
			ASSERT_LOG(false, "grid: header_rows must be an int or list of ints");
		}
	}
	if(v.has_key("horizontal_padding")) {
		set_hpad(v["horizontal_padding"].as_int());
	}
	if(v.has_key("show_background")) {
		show_background_ = v["show_background"].as_bool();
	}

	if(v.has_key("children")) {
		// children is a list of lists or a list of single widgets, the outmost list being rows, 
		// the inner list being the columns. 
		foreach(const variant& row, v["children"].as_list()) {
			if(row.is_list()) {
				foreach(const variant& col, row.as_list()) {
					add_col(widget_factory::create(v,e));
				}
				finish_row();
			} else {
				add_col(widget_factory::create(v,e))
					.finish_row();
			}
		}
	}

	set_h_ = height();
	set_w_ = width();
}

void grid::set_dim(int w, int h)
{
	widget::set_dim(w,h);
	set_h_ = h;
	set_w_ = w;
}

void grid::handle_process()
{
	widget::handle_process();
    foreach(widget_ptr w, cells_) {
		w->process();
	}
}

void grid::add_row(const std::vector<widget_ptr>& widgets)
{
	assert(widgets.size() == ncols_);
	int index = 0;
	foreach(const widget_ptr& widget, widgets) {
		cells_.push_back(widget);

		if(widget && widget->width()+hpad_ > col_widths_[index]) {
			col_widths_[index] = widget->width()+hpad_;
		}

		if(widget && widget->height() > row_height_) {
			row_height_ = widget->height();
		}

		++index;
	}

	recalculate_dimensions();
}

grid& grid::add_col(const widget_ptr& widget) {
	new_row_.push_back(widget);
	if(new_row_.size() == ncols_) {
		add_row(new_row_);
		new_row_.clear();
	}
	return *this;
}

grid& grid::finish_row()
{
	while(!new_row_.empty()) {
		add_col();
	}

	return *this;
}

grid& grid::set_col_width(int col, int width)
{
	assert(col >= 0 && col < ncols_);
	col_widths_[col] = width;
	recalculate_dimensions();
	return *this;
}

grid& grid::set_align(int col, grid::COLUMN_ALIGN align)
{
	assert(col >= 0 && col < ncols_);
	col_aligns_[col] = align;
	recalculate_dimensions();
	return *this;
}

grid& grid::set_hpad(int pad)
{
	hpad_ = pad;
	return *this;
}

void grid::register_mouseover_callback(grid::callback_type ptr)
{
	on_mouseover_ = ptr;
}

void grid::register_selection_callback(grid::callback_type ptr)
{
	on_select_ = ptr;
}

void grid::register_row_selection_callback(boost::function<void()> ptr)
{
	row_callbacks_.push_back(ptr);
}

int grid::row_at(int xpos, int ypos) const
{
	if(row_height_ == 0) {
		return -1;
	} else if(xpos > x() && xpos < x() + width() &&
	   ypos > y() && ypos < y() + height()) {
		return (ypos + yscroll() - y()) / row_height_;
	} else {
		return -1;
	}
}

void grid::recalculate_dimensions()
{
	visible_cells_.clear();

	int w = 0;
	foreach(int width, col_widths_) {
		w += width;
	}

	int desired_height = row_height_*nrows();
	set_virtual_height(desired_height);
	set_scroll_step(row_height_);

	if(max_height_ > 0 && desired_height > max_height_) {
		desired_height = max_height_;
		while(desired_height%row_height_) {
			--desired_height;
		}
	}

	if(set_h_ != 0 || set_w_ != 0) {
		widget::set_dim(set_w_ ? set_w_ : w, set_h_ ? set_h_ : desired_height);
	} else {
		widget::set_dim(w, desired_height);
	}

	int y = 0;
	for(int n = 0; n != nrows(); ++n) {
		int x = 0;
		for(int m = 0; m != ncols_; ++m) {
			int align = 0;
			widget_ptr widget = cells_[n*ncols_ + m];
			if(widget) {
				switch(col_aligns_[m]) {
				case ALIGN_LEFT:
					align = 0;
					break;
				case ALIGN_CENTER:
					align = (col_widths_[m] - widget->width())/2;
					break;
				case ALIGN_RIGHT:
					align = col_widths_[m] - widget->width();
					break;
				}

				widget->set_loc(x+align,y+row_height_/2 - widget->height()/2 - yscroll());
				if(widget->y() >= 0 && widget->y() < height()) {
					visible_cells_.push_back(widget);
				}
				std::sort(visible_cells_.begin(), visible_cells_.end(), widget_sort_zorder());
			}

			x += col_widths_[m];
		}

		y += row_height_;
	}

	update_scrollbar();
}

void grid::on_set_yscroll(int old_value, int value)
{
	recalculate_dimensions();
}

void grid::handle_draw() const
{
	glPushMatrix();
	glTranslatef(x() & ~1, y() & ~1, 0.0);
	if(show_background_) {
		const SDL_Color bg = {0,0,0};
		SDL_Rect rect = {0,0,width(),height()};
		graphics::draw_rect(rect,bg);
	}

	if(allow_highlight_ && selected_row_ >= 0 && selected_row_ < nrows()) {
		if(std::find(header_rows_.begin(), header_rows_.end(), selected_row_) == header_rows_.end()) {
			SDL_Rect rect = {0,row_height_*selected_row_ - yscroll(),width(),row_height_};
			const SDL_Color col = {0xFF,0x00,0x00,0x00};
			graphics::draw_rect(rect,col,128);
		}
	}
	foreach(const widget_ptr& widget, visible_cells_) {
		if(widget) {
			widget->draw();
		}
	}
	glPopMatrix();

	scrollable_widget::handle_draw();
}

bool grid::handle_event(const SDL_Event& event, bool claimed)
{
	claimed = scrollable_widget::handle_event(event, claimed);

	SDL_Event ev = event;
	normalize_event(&ev);
	std::vector<widget_ptr> cells = visible_cells_;
	reverse_foreach(widget_ptr widget, cells) {
		if(widget) {
			claimed = widget->process_event(ev, claimed);
		}
	}

	if(!claimed && allow_selection_) {
		if(event.type == SDL_MOUSEMOTION) {
			const SDL_MouseMotionEvent& e = event.motion;
			int new_row = row_at(e.x,e.y);
			if(new_row != selected_row_) {
				selected_row_ = new_row;
				if(on_mouseover_) {
					on_mouseover_(new_row);
				}
			}
		} else if(event.type == SDL_MOUSEBUTTONDOWN) {
			point p(event.button.x, event.button.y);
			rect r(x(), y(), width(), height());
			if(event.button.button == SDL_BUTTON_WHEELUP && point_in_rect(p, r)) {
				set_yscroll(yscroll() - 3*row_height_ < 0 ? 0 : yscroll() - 3*row_height_);
				selected_row_ -= 3;
				if(selected_row_ < 0) {
					selected_row_ = 0;
				}
				claimed = true;
			} else if(event.button.button == SDL_BUTTON_WHEELDOWN  && point_in_rect(p, r)) {
				int y3 = yscroll() + 3*row_height_;
				set_yscroll(virtual_height() - y3 < height() 
					? virtual_height() - height()
					: y3);
				selected_row_ += 3;
				if(selected_row_ >= nrows()) {
					selected_row_ = nrows() - 1;
				}
				claimed = true;
			} else {
				const SDL_MouseButtonEvent& e = event.button;
				if(e.state == SDL_PRESSED) {
					const int row_index = row_at(e.x, e.y);
					std::cerr << "SELECT ROW: " << row_index << "\n";
					if(row_index >= 0 && row_index < row_callbacks_.size() &&
					   row_callbacks_[row_index]) {
					std::cerr << "ROW CB: " << row_index << "\n";
						row_callbacks_[row_index]();
					}

					if(on_select_) {
						on_select_(row_index);
					}
				}
			}
			if(swallow_clicks_) {
				claimed = true;
			}
		}
	}

	if(!claimed && must_select_) {
		if(event.type == SDL_KEYDOWN) {
			if(event.key.keysym.sym == SDLK_UP) {
				if(selected_row_-- == 0) {
					selected_row_ = nrows()-1;
				}
				claimed = true;
			} else if(event.key.keysym.sym == SDLK_DOWN) {
				if(++selected_row_ == nrows()) {
					selected_row_ = 0;
				}
				claimed = true;
			}
		}
	}

	return claimed;
}

bool grid::has_focus() const
{
	foreach(const widget_ptr& w, cells_) {
		if(w && w->has_focus()) {
			return true;
		}
	}

	return false;
}

void grid::select_delegate(int selection)
{
	if(get_environment()) {
		game_logic::map_formula_callable* callable = new game_logic::map_formula_callable(get_environment());
		callable->add("selection", variant(selection));
		variant v(callable);
		variant value = ffl_on_select_->execute(*callable);
		get_environment()->execute_command(value);
	} else {
		std::cerr << "grid::select_delegate() called without environment!" << std::endl;
	}
}

void grid::mouseover_delegate(int selection)
{
	if(get_environment()) {
		game_logic::map_formula_callable* callable = new game_logic::map_formula_callable(get_environment());
		callable->add("selection", variant(selection));
		variant v(callable);
		variant value = ffl_on_mouseover_->execute(*callable);
		get_environment()->execute_command(value);
	} else {
		std::cerr << "grid::mouseover_delegate() called without environment!" << std::endl;
	}
}

void grid::set_value(const std::string& key, const variant& v)
{
	widget::set_value(key, v);
}

variant grid::get_value(const std::string& key) const
{
	if(key == "children") {
		std::vector<variant> v;
	    foreach(widget_ptr w, cells_) {
			v.push_back(variant(w.get()));
		}
		return variant(&v);
	}
	return widget::get_value(key);
}

}
