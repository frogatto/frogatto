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
#ifndef SELECTOR_WIDGET_HPP_INCLUDED
#define SELECTOR_WIDGET_HPP_INCLUDED

#include <vector>
#include <boost/intrusive_ptr.hpp>
#include <boost/function.hpp>

#include "widget.hpp"

namespace gui
{
typedef std::pair<std::string, widget_ptr> selector_pair;
typedef std::vector<selector_pair> selector_list;

class selector_widget : virtual public widget
{
public:
	explicit selector_widget(const std::vector<std::string>& list);
	explicit selector_widget(const selector_list& list);
	explicit selector_widget(const variant& v, game_logic::formula_callable* e);
	virtual ~selector_widget() {}

	void set_on_change_handler(boost::function<void(const std::string&)> fn) { on_change_ = fn; }
	void set_on_select_handler(boost::function<void(const std::string&)> fn) { on_select_ = fn; }
	void set_selection(const std::string& sel);
	void set_selection(size_t sel);
	std::string get_selection();
protected:
	virtual void handle_draw() const;
	virtual bool handle_event(const SDL_Event& event, bool claimed);

	virtual void set_value(const std::string& key, const variant& v);
	virtual variant get_value(const std::string& key) const;
	void init();
private:
	bool handle_mousedown(const SDL_MouseButtonEvent& event, bool claimed);
	bool handle_mouseup(const SDL_MouseButtonEvent& event, bool claimed);
	bool handle_mousemotion(const SDL_MouseMotionEvent& event, bool claimed);
	void select_left(size_t n=1);
	void select_right(size_t n=1);

	selector_list list_;
	size_t current_selection_;
	boost::function<void(const std::string&)> on_change_;
	boost::function<void(const std::string&)> on_select_;

	widget_ptr left_arrow_;
	widget_ptr right_arrow_;

	// delgate 
	void change_delegate(const std::string& s);
	void select_delegate(const std::string& s);
	// FFL formula
	game_logic::formula_ptr change_handler_;
	game_logic::formula_ptr select_handler_;
};

typedef boost::intrusive_ptr<selector_widget> selector_widget_ptr;
typedef boost::intrusive_ptr<const selector_widget> const_selector_widget_ptr;
}

#endif