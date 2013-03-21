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
#ifndef GUI_SECTION_HPP_INCLUDED
#define GUI_SECTION_HPP_INCLUDED

#include <string>

#include <boost/shared_ptr.hpp>

#include "geometry.hpp"
#include "texture.hpp"
#include "variant.hpp"

class gui_section;
typedef boost::shared_ptr<const gui_section> const_gui_section_ptr;

class gui_section
{
public:
	static void init(variant node);
	static const_gui_section_ptr get(const std::string& key);
	static const_gui_section_ptr get(const variant& v);

	explicit gui_section(variant node);

	void blit(int x, int y) const { blit(x, y, width(), height()); }
	void blit(int x, int y, int w, int h) const;
	int width() const { return area_.w()*2; }
	int height() const { return area_.h()*2; }
private:
	graphics::texture texture_;
	rect area_;
	rect draw_area_;

	int x_adjust_, y_adjust_, x2_adjust_, y2_adjust_;
};

#endif
