#ifndef ANIMATION_PREVIEW_WIDGET_HPP_INCLUDED
#define ANIMATION_PREVIEW_WIDGET_HPP_INCLUDED

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

private:
	void handle_draw() const;
	bool handle_event(const SDL_Event& event, bool claimed);

	void zoom_in();
	void zoom_out();

	variant obj_;

	boost::scoped_ptr<frame> frame_;
	mutable int cycle_;

	std::vector<widget_ptr> widgets_;
	mutable gui::label* zoom_label_;
	gui::label* pos_label_;

	mutable int scale_;
	void update_zoom_label() const;

	mutable rect src_rect_, dst_rect_;
};

}

#endif
