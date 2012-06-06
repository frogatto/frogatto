#include <sstream>

#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>

#include "font.hpp"
#include "foreach.hpp"
#include "label.hpp"
#include "poly_line_widget.hpp"
#include "raster.hpp"
#include "tree_view_widget.hpp"

namespace gui {

tree_view_widget::tree_view_widget(int w, int h, const variant& tree)
	: tree_(tree), hpad_(10), col_size_(80), font_size_(10), selected_row_(-1), 
	char_height_(font::char_height(font_size_)), allow_selection_(false),
	row_height_(font::char_height(font_size_)), must_select_(false), nrows_(0),
	max_height_(-1)
{
	set_environment();
	widget::set_dim(w, h);
	traverse(40, 0, variant(), &tree_);
	recalculate_dimensions();
}

tree_view_widget::tree_view_widget(const variant& v, game_logic::formula_callable* e)
	: scrollable_widget(v,e), selected_row_(-1), nrows_(0)
{
	tree_ = v["child"];

	hpad_ = v["horizontal_padding"].as_int(10);
	col_size_ = v["column_size"].as_int(80);
	font_size_ = v["font_size"].as_int(10);
	allow_selection_ = v["allow_selection"].as_bool(false);
	must_select_ = v["must_select"].as_bool(false);
	max_height_ = v["max_height"].as_int(-1);

	char_height_ = font::char_height(font_size_);
	row_height_ = font::char_height(font_size_);

	traverse(col_size_/2, 0, variant(), &tree_);
	recalculate_dimensions();
}

int tree_view_widget::traverse(int x, int y, const variant& key, variant* value)
{
	std::vector<point> points;
	std::map<int,int>::iterator it = last_coords_.find(x);
	int last_y = y;
	if(it != last_coords_.end()) {
		last_y = it->second;
	}
	points.push_back(point(x/2, last_y));
	points.push_back(point(x/2, y+char_height_/2));
	points.push_back(point(x, y+char_height_/2 ));
	poly_line_widget_ptr plw(new poly_line_widget(&points, graphics::color_grey()));
	widgets_.push_back(plw);
	last_coords_[x] = y + char_height_/2;

	label_ptr key_label;
	if(key.is_null() == false) {
		key_label.reset(new label(key.as_string(), graphics::color_white(), font_size_));
		key_label->set_loc(x, y);
		key_label->set_dim(col_size_, key_label->height());
		x += col_size_ + hpad_;
		widgets_.push_back(key_label);
	}
	if(value->is_null()) {
		label_ptr null_label(new label("<null>", graphics::color_yellow(), font_size_));
		null_label->set_loc(x, y);
		null_label->set_dim(col_size_, null_label->height());
		widgets_.push_back(null_label);
		y += widgets_.back()->height();
	} else if(value->is_int()) {
		std::stringstream ss;
		ss << value->as_int();
		label_ptr int_label(new label(ss.str(), graphics::color_yellow(), font_size_));
		int_label->set_loc(x, y);
		int_label->set_dim(col_size_, int_label->height());
		widgets_.push_back(int_label);
		y += widgets_.back()->height();
	} else if(value->is_decimal()) {
		std::stringstream ss;
		ss << value->as_decimal();
		label_ptr decimal_label(new label(ss.str(), graphics::color_yellow(), font_size_));
		decimal_label->set_loc(x, y);
		decimal_label->set_dim(col_size_, decimal_label->height());
		widgets_.push_back(decimal_label);
		y += widgets_.back()->height();
	} else if(value->is_string()) {
		label_ptr string_label(new label(value->as_string(), graphics::color_yellow(), font_size_));
		string_label->set_loc(x, y);
		string_label->set_dim(col_size_, string_label->height());
		widgets_.push_back(string_label);
		y += widgets_.back()->height();
	} else if(value->is_list()) {
		if(key_label) {
			y += key_label->height();
		}
		foreach(variant item, value->as_list()) {
			y = traverse(x, y, variant(), &item);
		}
		last_coords_.erase(x);
	} else if(value->is_map()) {
		if(key_label) {
			y += key_label->height();
		}
		std::pair<variant, variant> p;
		foreach(p, value->as_map()) {
			y = traverse(x, y, p.first, &p.second);
		}
		last_coords_.erase(x);
	} else if(value->is_bool()) {
		label_ptr bool_label(new label(value->as_bool() ? "true" : "false", graphics::color_yellow(), font_size_));
		bool_label->set_loc(x, y);
		bool_label->set_dim(col_size_, bool_label->height());
		widgets_.push_back(bool_label);
		y += widgets_.back()->height();
	}
	//row_map_[nrows_] = std::pair<std::string, variant_ptr>(key.is_string() ? key.as_string() : "", value);
	nrows_++;
	return y;
}

void tree_view_widget::handle_draw() const
{
	graphics::draw_hollow_rect(
		rect(x(), y(), width(), height()).sdl_rect(), 
		graphics::color_grey());

	glPushMatrix();
	glTranslatef(GLfloat(x()+2 & ~1), GLfloat(y()+2 & ~1), 0.0);

	if(selected_row_ >= 0 && selected_row_ < nrows()) {
		SDL_Rect rect = {0,row_height_*selected_row_ - yscroll(),width(),row_height_};
		const SDL_Color col = {0xFF,0x00,0x00,0x00};
		graphics::draw_rect(rect,col,128);
	}

	glTranslatef(0, GLfloat(-yscroll() & ~1), 0.0);

	foreach(const widget_ptr& w, widgets_) {
		w->draw();
	}
	glPopMatrix();

	scrollable_widget::handle_draw();
}

int tree_view_widget::row_at(int xpos, int ypos) const
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

void tree_view_widget::recalculate_dimensions()
{
	int desired_height = row_height_*nrows();
	set_virtual_height(desired_height);
	set_scroll_step(row_height_);
	
	if(max_height_ > 0 && desired_height > max_height_) {
		desired_height = max_height_;
		while(desired_height%row_height_) {
			--desired_height;
		}
	}

	foreach(const widget_ptr& w, widgets_) {
		if(w->y() - yscroll() >= 0 && w->y() + w->height() - yscroll() < height()) {
			w->set_visible(true);
		} else {
			w->set_visible(false);
		}
	}
	
	update_scrollbar();
}

void tree_view_widget::on_set_yscroll(int old_value, int value)
{
	recalculate_dimensions();
}


bool tree_view_widget::handle_event(const SDL_Event& event, bool claimed)
{
	claimed = scrollable_widget::handle_event(event, claimed);
	if(!claimed && allow_selection_) {
		if(event.type == SDL_MOUSEMOTION) {
			const SDL_MouseMotionEvent& e = event.motion;
			int new_row = row_at(e.x,e.y);
			if(new_row != selected_row_) {
				selected_row_ = new_row;
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
				const int row_index = row_at(e.x, e.y);
				std::cerr << "TREEVIEW SELECT ROW: " << row_index << "\n";

				if(on_select_) {
					//on_select_(row_index, row_map_[row_index]);
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
	//std::vector<widget_ptr> cells = visible_cells_;
	//reverse_foreach(widget_ptr widget, cells) {
	//	if(widget) {
	//		claimed = widget->process_event(ev, claimed);
	//	}
	//}
	return claimed;
}

void tree_view_widget::set_value(const std::string& key, const variant& v)
{
	widget::set_value(key, v);
}

variant tree_view_widget::get_value(const std::string& key) const
{
	return widget::get_value(key);
}

}