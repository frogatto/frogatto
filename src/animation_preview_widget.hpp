#ifndef ANIMATION_PREVIEW_WIDGET_HPP_INCLUDED
#define ANIMATION_PREVIEW_WIDGET_HPP_INCLUDED

#include <boost/function.hpp>
#include <boost/scoped_ptr.hpp>

#include "frame.hpp"
#include "label.hpp"
#include "variant.hpp"
#include "widget.hpp"

namespace gui {

class animation_preview_widget : public widget
{
public:
	static bool is_animation(variant obj);

	explicit animation_preview_widget(variant obj);
	void init();
	void set_object(variant obj);

	void process();

	void set_rect_handler(boost::function<void(rect)>);

private:
	void handle_draw() const;
	bool handle_event(const SDL_Event& event, bool claimed);

	void zoom_in();
	void zoom_out();

	point mouse_point_to_image_loc(const point& p) const;

	variant obj_;

	boost::scoped_ptr<frame> frame_;
	mutable int cycle_;

	std::vector<widget_ptr> widgets_;
	mutable gui::label* zoom_label_;
	gui::label* pos_label_;

	mutable int scale_;
	void update_zoom_label() const;

	mutable rect src_rect_, dst_rect_;

	//anchors for mouse dragging events.
	int anchor_x_, anchor_y_;

	boost::function<void(rect)> rect_handler_;
};

}

#endif
