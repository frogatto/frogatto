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
#ifndef PROGRESS_BAR_HPP_INCLUDED
#define PROGRESS_BAR_HPP_INCLUDED

#include <boost/bind.hpp>
#include <boost/function.hpp>

#include "color_utils.hpp"
#include "framed_gui_element.hpp"
#include "texture.hpp"
#include "widget.hpp"

namespace gui {

class progress_bar : public widget
{
public:
	progress_bar(int progress=0, int minv=0, int maxv=100, const std::string& gui_set="default_button");
	explicit progress_bar(const variant& v, game_logic::formula_callable* e);

	int min_value() const { return min_; }
	int max_value() const { return max_; }
	int progress() const { return progress_; }
	void set_progress(int value);
	void update_progress(int delta);
	void set_completion_handler(boost::function<void ()> oncompletion);
	void reset();
protected:
	virtual variant get_value(const std::string& key) const;
	virtual void set_value(const std::string& key, const variant& value);
	void handle_draw() const;
private:
	void complete();

	graphics::color color_;
	int hpad_;
	int vpad_;
	int min_;
	int max_;
	int progress_;
	bool completion_called_;
	boost::function<void ()> oncompletion_;
	game_logic::formula_ptr completion_handler_;

	bool upscale_;
	const_framed_gui_element_ptr frame_image_set_;
};

typedef boost::intrusive_ptr<progress_bar> progress_bar_ptr;
typedef boost::intrusive_ptr<const progress_bar> const_progress_bar_ptr;

}

#endif
