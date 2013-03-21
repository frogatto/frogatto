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
#ifndef PREVIEW_TILESET_WIDGET_HPP_INCLUDED
#define PREVIEW_TILESET_WIDGET_HPP_INCLUDED

#include "level_object.hpp"
#include "widget.hpp"

class tile_map;

namespace gui {

class preview_tileset_widget : public widget
{
public:
	explicit preview_tileset_widget(const tile_map& tiles);
	explicit preview_tileset_widget(const variant& v, game_logic::formula_callable* e);
protected:
	void init();
	virtual void set_value(const std::string& key, const variant& v);
	virtual variant get_value(const std::string& key) const;
private:
	void handle_draw() const;
	std::vector<level_tile> tiles_;
	int width_, height_;
};

typedef boost::intrusive_ptr<preview_tileset_widget> preview_tileset_widget_ptr;

}

#endif
