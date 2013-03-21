/*
	Copyright (C) 2003-2013 by David White <davewx7@gmail.com>
	
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
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
