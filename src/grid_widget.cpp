
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

#include "foreach.hpp"
#include "grid_widget.hpp"
#include "raster.hpp"

namespace gui {

grid::grid(int ncols)
  : ncols_(ncols), col_widths_(ncols, 0),
    col_aligns_(ncols, grid::ALIGN_LEFT), row_height_(0),
	selected_row_(-1), allow_selection_(false), must_select_(false),
    swallow_clicks_(false), hpad_(0), show_background_(false),
	max_height_(-1)
{
	set_dim(0,0);
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
	if(xpos > x() && xpos < x() + width() &&
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

	set_dim(w, desired_height);

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

	if(selected_row_ >= 0 && selected_row_ < nrows()) {
		SDL_Rect rect = {0,row_height_*selected_row_ - yscroll(),width(),row_height_};
		const SDL_Color col = {0xFF,0x00,0x00,0x00};
		graphics::draw_rect(rect,col,128);
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
			if(event.button.button == SDL_BUTTON_WHEELUP) {
				set_yscroll(yscroll() - 3*row_height_ < 0 ? 0 : yscroll() - 3*row_height_);
				claimed = true;
			} else if(event.button.button == SDL_BUTTON_WHEELDOWN) {
				int y3 = yscroll() + 3*row_height_;
				set_yscroll(virtual_height() - y3 < height() 
					? virtual_height() - height()
					: y3);
				claimed = true;
			} else {
				const SDL_MouseButtonEvent& e = event.button;
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

	SDL_Event ev = event;
	normalize_event(&ev);
	foreach(const widget_ptr& widget, visible_cells_) {
		if(widget) {
			claimed = widget->process_event(ev, claimed);
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

}
