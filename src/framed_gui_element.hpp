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
#ifndef FRAMED_GUI_ELEMENT_HPP_INCLUDED
#define FRAMED_GUI_ELEMENT_HPP_INCLUDED

#include <boost/shared_ptr.hpp>
#include "geometry.hpp"
#include "texture.hpp"
#include "variant.hpp"

class framed_gui_element;
typedef boost::shared_ptr<const framed_gui_element> const_framed_gui_element_ptr;


class framed_gui_element
{
public:
	static void init(variant node);
	static const_framed_gui_element_ptr get(const std::string& key);

	void blit(int x, int y, int w, int h, bool upscaled = 0) const;
	explicit framed_gui_element(variant node);

	int corner_height() const { return corner_height_; }
	
private:
	void blit_subsection(rect subsection, int x, int y, int w, int h) const;
	
	const rect area_;
	const int corner_height_;
	graphics::texture texture_;
	
	rect  top_right_corner_;
	rect  top_left_corner_;
	rect  bottom_right_corner_;
	rect  bottom_left_corner_;
	
	rect  top_border_;
	rect  bottom_border_;
	rect  left_border_;
	rect  right_border_;
	
	
	rect interior_fill_;  //later on, we might want to do this as a proper pattern.  For now, we've imped this as a stretch-to-fill because it doesn't matter with our current graphics (since they're just a solid color).  If we never get anything but solid colors, no need to waste the effort.

};

#endif
