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
#ifndef GRAPHICAL_FONT_HPP_INCLUDED
#define GRAPHICAL_FONT_HPP_INCLUDED

#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>

#include <string>
#include <vector>

#include "geometry.hpp"
#include "texture.hpp"
#include "variant.hpp"

class graphical_font;
typedef boost::shared_ptr<graphical_font> graphical_font_ptr;
typedef boost::shared_ptr<const graphical_font> const_graphical_font_ptr;

class graphical_font
{
public:
	static void init(variant node);
	static void init_for_locale(const std::string& locale);
	static const_graphical_font_ptr get(const std::string& id);
	explicit graphical_font(variant node);
	const std::string& id() const { return id_; }
	rect draw(int x, int y, const std::string& text, int size=2) const;
	rect dimensions(const std::string& text, int size=2) const;

private:
	rect do_draw(int x, int y, const std::string& text, bool draw_text, int size) const;

	std::string id_;

	graphics::texture texture_;
	//hashmap to map characters to rectangles in the texture
	typedef boost::unordered_map<unsigned int, rect> char_rect_map;
	char_rect_map char_rect_map_;
	int kerning_;
};

#endif
