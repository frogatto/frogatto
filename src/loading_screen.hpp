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
#ifndef LOADING_SCREEN_HPP_INCLUDED
#define LOADING_SCREEN_HPP_INCLUDED

#include <string>

#include "graphical_font.hpp"
#include "texture.hpp"
#include "variant.hpp"

class loading_screen
{
	public:
	loading_screen (int items=0);
	void load(variant node); // preload objects defined by preload children of node, blocking, and calling draw automatically
	void draw (const std::string& message);
	void increment_status ();
	void draw_and_increment (const std::string& message) {draw(message); increment_status();}
	void set_number_of_items (int items);

	void finish_loading();
	
	private:
	void draw_internal(const std::string& message);
	int items_; // number of items we'll load
	int status_; // how many items we've loaded so far
	graphics::texture background_;
	graphics::texture splash_;

	int started_at_;
};

#endif
