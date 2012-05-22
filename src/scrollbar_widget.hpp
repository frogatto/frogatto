#ifndef SCROLLBAR_WIDGET_HPP_INCLUDED
#define SCROLLBAR_WIDGET_HPP_INCLUDED

#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>

#include "widget.hpp"

namespace gui
{

class scrollbar_widget : public gui::widget
{
public:
	explicit scrollbar_widget(boost::function<void(int)> handler);

	void set_range(int total_height, int window_height);
	void set_loc(int x, int y);
	void set_dim(int w, int h);
	void set_window_pos(int pos) { window_pos_ = pos; }
	void set_step(int step) { step_ = step; }
	int window_pos() const { return window_pos_; }
private:
	void handle_draw() const;
	bool handle_event(const SDL_Event& event, bool claimed);

	void down_button_pressed();
	void up_button_pressed();

	boost::function<void(int)> handler_;
	widget_ptr up_arrow_, down_arrow_, handle_, handle_bot_, handle_top_, background_;
	int window_pos_, window_size_, range_, step_;

	bool dragging_handle_;
	int drag_start_;
	int drag_anchor_y_;
};

typedef boost::intrusive_ptr<scrollbar_widget> scrollbar_widget_ptr;

}

#endif
