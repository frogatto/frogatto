#ifndef ANIMATION_PREVIEW_WIDGET_HPP_INCLUDED
#define ANIMATION_PREVIEW_WIDGET_HPP_INCLUDED
#ifndef NO_EDITOR

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
	explicit animation_preview_widget(const variant& v, game_logic::formula_callable* e);
	void init();
	void set_object(variant obj);

	void process();

	void set_rect_handler(boost::function<void(rect)>);
	void set_pad_handler(boost::function<void(int)>);
	void set_num_frames_handler(boost::function<void(int)>);
	void set_frames_per_row_handler(boost::function<void(int)>);
	void set_solid_handler(boost::function<void(int,int)>);

protected:
	virtual void set_value(const std::string& key, const variant& v);
	virtual variant get_value(const std::string& key) const;

private:
	void handle_draw() const;
	bool handle_event(const SDL_Event& event, bool claimed);

	void zoom_in();
	void zoom_out();
	void reset_rect();

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
	rect anchor_area_;
	int anchor_pad_;
	bool has_motion_;

	mutable rect locked_focus_;

	mutable int dragging_sides_bitmap_;
	enum { LEFT_SIDE = 1, RIGHT_SIDE = 2, TOP_SIDE = 4, BOTTOM_SIDE = 8, PADDING = 16 };

	mutable rect solid_rect_;
	bool moving_solid_rect_;
	int anchor_solid_x_, anchor_solid_y_;

	boost::function<void(rect)> rect_handler_;
	boost::function<void(int)> pad_handler_;
	boost::function<void(int)> num_frames_handler_;
	boost::function<void(int)> frames_per_row_handler_;
	boost::function<void(int,int)> solid_handler_;

	void rect_handler_delegate(rect r);
	void pad_handler_delegate(int pad);
	void num_frames_handler_delegate(int frames);
	void frames_per_row_handler_delegate(int frames_per_row);
	void solid_handler_delegate(int x, int y);

	game_logic::formula_ptr ffl_rect_handler_;
	game_logic::formula_ptr ffl_pad_handler_;
	game_logic::formula_ptr ffl_num_frames_handler_;
	game_logic::formula_ptr ffl_frames_per_row_handler_;
	game_logic::formula_ptr ffl_solid_handler_;
};

typedef boost::intrusive_ptr<gui::animation_preview_widget> animation_preview_widget_ptr;

}

#endif // !NO_EDITOR
#endif
