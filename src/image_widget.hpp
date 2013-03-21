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
#ifndef IMAGE_WIDGET_HPP_INCLUDED
#define IMAGE_WIDGET_HPP_INCLUDED

#include <string>

#include "geometry.hpp"
#include "gui_section.hpp"
#include "image_widget_fwd.hpp"
#include "texture.hpp"
#include "widget.hpp"

namespace gui {

class image_widget : public widget
{
public:
	explicit image_widget(const std::string& fname, int w=-1, int h=-1);
	explicit image_widget(graphics::texture tex, int w=-1, int h=-1);
	explicit image_widget(const variant& v, game_logic::formula_callable* e);

	void init(int w, int h);

	void set_rotation(GLfloat rotate) { rotate_ = rotate; }
	void set_area(const rect& area) { area_ = area; }

	void set_value(const std::string& key, const variant& v);
	variant get_value(const std::string& key) const;

private:
	void handle_draw() const;

	graphics::texture texture_;
	GLfloat rotate_;
	rect area_;
	std::string image_name_;
};

class gui_section_widget : public widget
{
public:
	explicit gui_section_widget(const std::string& id, int w=-1, int h=-1, int scale=1);
	explicit gui_section_widget(const variant& v, game_logic::formula_callable* e);

	//sets the GUI section. The dimensions of the widget will not change;
	//you should set a GUI section that is the same size.
	void set_gui_section(const std::string& id);

	void handle_draw() const;
protected:
	void set_value(const std::string& key, const variant& v);
	variant get_value(const std::string& key) const;
private:
	const_gui_section_ptr section_;
	int scale_;
};

typedef boost::intrusive_ptr<gui_section_widget> gui_section_widget_ptr;

}

#endif
