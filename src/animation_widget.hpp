#pragma once
#ifndef ANIMATION_WIDGET_HPP_INCLUDED
#define ANIMATION_WIDGET_HPP_INCLUDED

#include <boost/scoped_ptr.hpp>
#include <boost/intrusive_ptr.hpp>

#include "frame.hpp"
#include "geometry.hpp"
#include "label.hpp"
#include "widget.hpp"

namespace gui {

class animation_widget : public virtual widget
{
public:
	animation_widget(int w, int h, const variant& node);
	animation_widget(const variant& v, game_logic::formula_callable* e);

	void set_sequence_play_count(int count) { max_sequence_plays_ = count; }
protected:
	virtual void handle_draw() const;
private:
	void init();

	std::vector<variant> nodes_;
	mutable label_ptr label_;
	mutable boost::scoped_ptr<frame> frame_;
	mutable int cycle_;
	mutable int play_sequence_count_;
	// Number of times to repeat play each animation sequence.
	int max_sequence_plays_;
	mutable std::vector<variant>::const_iterator current_anim_;
};

typedef boost::intrusive_ptr<animation_widget> animation_widget_ptr;
typedef boost::intrusive_ptr<const animation_widget> const_animation_widget_ptr;

}

#endif 
