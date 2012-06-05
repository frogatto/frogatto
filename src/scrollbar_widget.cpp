#include <boost/bind.hpp>

#include "image_widget.hpp"
#include "scrollbar_widget.hpp"
#include "widget_factory.hpp"

namespace gui {

namespace {
const std::string UpArrow = "scrollbar-vertical-up-arrow";
const std::string DownArrow = "scrollbar-vertical-down-arrow";
const std::string VerticalHandle = "scrollbar-vertical-handle-middle";
const std::string VerticalHandleBot = "scrollbar-vertical-handle-bottom";
const std::string VerticalHandleTop = "scrollbar-vertical-handle-top";
const std::string VerticalBackground = "scrollbar-vertical-background";
}

scrollbar_widget::scrollbar_widget(boost::function<void(int)> handler)
  : handler_(handler),
    up_arrow_(new gui_section_widget(UpArrow)),
    down_arrow_(new gui_section_widget(DownArrow)),
	handle_(new gui_section_widget(VerticalHandle)),
	handle_bot_(new gui_section_widget(VerticalHandleBot)),
	handle_top_(new gui_section_widget(VerticalHandleTop)),
	background_(new gui_section_widget(VerticalBackground)),
	
	window_pos_(0), window_size_(0), range_(0), step_(0),
	dragging_handle_(false),
	drag_start_(0), drag_anchor_y_(0)
{}

scrollbar_widget::scrollbar_widget(const variant& v, const game_logic::formula_callable_ptr& e)
	: widget(v,e),	window_pos_(0), window_size_(0), range_(0), step_(0),
	dragging_handle_(false), drag_start_(0), drag_anchor_y_(0)
{
	handler_ = boost::bind(&scrollbar_widget::handler_delegate, this, _1);
	// XXX replace the 0 with an actual symbol table.
	ffl_handler_ = game_logic::formula_ptr(new game_logic::formula(v["on_scroll"], 0));
	
    up_arrow_ = v.has_key("up_arrow") ? widget_factory::create(v["up_arrow"], e) : new gui_section_widget(UpArrow);
    down_arrow_ = v.has_key("down_arrow") ? widget_factory::create(v["down_arrow"], e) : new gui_section_widget(DownArrow);
	handle_ = v.has_key("handle") ? widget_factory::create(v["handle"], e) : new gui_section_widget(VerticalHandle);
	handle_bot_ = v.has_key("handle_bottom") ? widget_factory::create(v["handle_bottom"], e) : new gui_section_widget(VerticalHandleBot);
	handle_top_ = v.has_key("handle_top") ? widget_factory::create(v["handle_top"], e) : new gui_section_widget(VerticalHandleTop);
	background_ = v.has_key("background") ? widget_factory::create(v["background"], e) : new gui_section_widget(VerticalBackground);
	if(v.has_key("range")) {
		std::vector<int> range = v["range"].as_list_int();
		ASSERT_EQ(range.size(), 2);
		set_range(range[0], range[1]);
	}
}

void scrollbar_widget::handler_delegate(int yscroll)
{
	game_logic::map_formula_callable* callable = new game_logic::map_formula_callable(get_environment().get());
	callable->add("yscroll", variant(yscroll));
	variant v(callable);
	variant value = ffl_handler_->execute(*callable);
	value.try_convert<game_logic::command_callable>()->execute(*callable);
}

void scrollbar_widget::set_range(int total_height, int window_height)
{
	window_size_ = window_height;
	range_ = total_height;
	if(window_pos_ < 0 || window_pos_ > range_ - window_size_) {
		window_pos_ = range_ - window_size_;
	}
}

void scrollbar_widget::set_loc(int x, int y)
{
	widget::set_loc(x, y);
	set_dim(width(), height());
}

void scrollbar_widget::set_dim(int w, int h)
{
	w = up_arrow_->width();
	up_arrow_->set_loc(x(), y());
	down_arrow_->set_loc(x(), y() + h - down_arrow_->height());
	background_->set_loc(x(), y() + up_arrow_->height());

	const int bar_height = h - (down_arrow_->height() + up_arrow_->height());
	background_->set_dim(background_->width(), bar_height);

	if(range_) {
		handle_->set_loc(x(), y() + up_arrow_->height() + (window_pos_*bar_height)/range_);
		handle_->set_dim(handle_->width(), std::max<int>(6, (window_size_*bar_height)/range_));
		handle_top_->set_loc(x(), y()+ up_arrow_->height() + (window_pos_*bar_height)/range_);
		handle_bot_->set_loc(x(), y()+ down_arrow_->height() + (window_pos_*bar_height)/range_ + (window_size_*bar_height)/range_ - handle_bot_->height() +1);
	}

	//TODO:  handle range < heightOfEndcaps
	
	widget::set_dim(w, h);
}

void scrollbar_widget::down_button_pressed()
{
}

void scrollbar_widget::up_button_pressed()
{
}

void scrollbar_widget::handle_draw() const
{
	up_arrow_->draw();
	down_arrow_->draw();
	background_->draw();
	handle_->draw();
	handle_bot_->draw();
	handle_top_->draw();
}

void scrollbar_widget::clip_window_position()
{
	if(window_pos_ < 0) {
		window_pos_ = 0;
	}

	if(window_pos_ > range_ - window_size_) {
		window_pos_ = range_ - window_size_;
	}
}

bool scrollbar_widget::handle_event(const SDL_Event& event, bool claimed)
{
	if(claimed) {
		return claimed;
	}

	if(event.type == SDL_MOUSEBUTTONDOWN) {
		const SDL_MouseButtonEvent& e = event.button;
		if(e.x < x() || e.x > x() + width() ||
		   e.y < y() || e.y > y() + height()) {
			return claimed;
		}

		const int start_pos = window_pos_;

		claimed = true;

		if( e.button == SDL_BUTTON_WHEELUP || e.button == SDL_BUTTON_WHEELDOWN ) {
			if(e.button == SDL_BUTTON_WHEELUP) {
				window_pos_ -= 3 * step_;
			} else if(e.button == SDL_BUTTON_WHEELDOWN) {
				window_pos_ += 3 * step_;
			}

			clip_window_position();

			if(window_pos_ != start_pos) {
				set_dim(width(), height());
				handler_(window_pos_);
			}
			return claimed;
		}

		if(e.y < up_arrow_->y() + up_arrow_->height()) {
			//on up arrow
			window_pos_ -= step_;
		} else if(e.y > down_arrow_->y()) {
			//on down arrow
			window_pos_ += step_;
		} else if(e.y < handle_->y()) {
			//page up
			window_pos_ -= window_size_ - step_;
		} else if(e.y > handle_->y() + handle_->height()) {
			//page down
			window_pos_ += window_size_ - step_;
		} else {
			//on handle
			dragging_handle_ = true;
			drag_start_ = window_pos_;
			drag_anchor_y_ = e.y;
		}

		std::cerr << "HANDLE: " << handle_->y() << ", " << handle_->height() << "\n";

		clip_window_position();

		if(window_pos_ != start_pos) {
			set_dim(width(), height());
			handler_(window_pos_);
		}

	} else if(event.type == SDL_MOUSEBUTTONUP) {
		dragging_handle_ = false;
	} else if(event.type == SDL_MOUSEMOTION) {
		const SDL_MouseMotionEvent& e = event.motion;

		int mousex, mousey;
		if(!SDL_GetMouseState(&mousex, &mousey)) {
			dragging_handle_ = false;
		}

		if(dragging_handle_) {
			const int handle_height = height() - up_arrow_->height() - down_arrow_->height();
			const int move = e.y - drag_anchor_y_;
			const int window_move = (move*range_)/handle_height;
			window_pos_ = drag_start_ + window_move;
			if(step_) {
				window_pos_ -= window_pos_%step_;
			}

			clip_window_position();

			set_dim(width(), height());
			handler_(window_pos_);
		}
	}


	return claimed;
}

void scrollbar_widget::set_value(const std::string& key, const variant& v)
{
	if(key == "on_scroll") {
		// XXX replace the 0 with an actual symbol table.
		ffl_handler_ = game_logic::formula_ptr(new game_logic::formula(v["on_scroll"], 0));
	} else if(key == "up_arrow") {
		up_arrow_ = widget_factory::create(v, get_environment());
	} else if(key == "down_arrow") {
		down_arrow_ = widget_factory::create(v, get_environment());
	} else if(key == "handle") {
		handle_ = widget_factory::create(v, get_environment());
	} else if(key == "handle_bottom") {
		handle_bot_ = widget_factory::create(v, get_environment());
	} else if(key == "handle_top") {
		handle_top_ = widget_factory::create(v, get_environment());
	} else if(key == "background") {
		background_ = widget_factory::create(v, get_environment());
	} else if(key == "range") {
		std::vector<int> range = v.as_list_int();
		ASSERT_EQ(range.size(), 2);
		set_range(range[0], range[1]);
	} else if(key == "position") {
		window_pos_ = v.as_int();
		clip_window_position();
	}
	
	widget::set_value(key, v);
}

variant scrollbar_widget::get_value(const std::string& key) const
{
	if(key == "range") {
		std::vector<variant> vv;
		vv.resize(2);
		vv.push_back(variant(range_));
		vv.push_back(variant(window_size_));
	} else if(key == "position") {
		return variant(window_pos_);
	}
	return widget::get_value(key);
}

}
