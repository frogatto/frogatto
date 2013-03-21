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
#ifndef IMAGE_WIDGET_FWD_HPP_INCLUDED
#define IMAGE_WIDGET_FWD_HPP_INCLUDED

#include <boost/shared_ptr.hpp>

namespace gui {

class image_widget;
typedef boost::intrusive_ptr<image_widget> image_widget_ptr;
typedef boost::intrusive_ptr<const image_widget> const_image_widget_ptr;

}

#endif
