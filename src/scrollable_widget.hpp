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
#ifndef SCROLLABLE_WIDGET_HPP_INCLUDED
#define SCROLLABLE_WIDGET_HPP_INCLUDED

#include <boost/shared_ptr.hpp>

#include "scrollbar_widget.hpp"
#include "widget.hpp"

namespace gui {

class scrollable_widget : virtual public widget
{
public:
	scrollable_widget();
	scrollable_widget(const variant& v, game_logic::formula_callable* e);
	void set_yscroll(int yscroll);
	virtual void set_dim(int w, int h);

	virtual void handle_draw() const;
	virtual bool handle_event(const SDL_Event& event, bool claimed);

	virtual void set_loc(int x, int y);
protected:
	~scrollable_widget();
	void set_virtual_height(int height);
	void set_scroll_step(int step);
	void update_scrollbar();

	int yscroll() const { return yscroll_; }
	int virtual_height() const { return virtual_height_; }

	virtual void set_value(const std::string& key, const variant& v);
	virtual variant get_value(const std::string& key) const;
private:
	virtual void on_set_yscroll(int old_yscroll, int new_yscroll);

	int yscroll_;
	int virtual_height_;
	int step_;

	scrollbar_widget_ptr scrollbar_;
};

typedef boost::intrusive_ptr<scrollable_widget> scrollable_widget_ptr;
typedef boost::intrusive_ptr<const scrollable_widget> const_scrollable_widget_ptr;

}

#endif
