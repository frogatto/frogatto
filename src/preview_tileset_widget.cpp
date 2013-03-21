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
#include <iostream>

#include "foreach.hpp"
#include "preview_tileset_widget.hpp"
#include "tile_map.hpp"

namespace gui {

preview_tileset_widget::preview_tileset_widget(const tile_map& tiles)
  : width_(0), height_(0)
{
	set_environment();
	tiles.build_tiles(&tiles_);
	init();
}

preview_tileset_widget::preview_tileset_widget(const variant& v, game_logic::formula_callable* e)
	: widget(v,e)
{
	tile_map(v["tile_map"]).build_tiles(&tiles_);
	init();
}

void preview_tileset_widget::init()
{
	foreach(const level_tile& t, tiles_) {
		const int w = t.x + t.object->width();
		const int h = t.y + t.object->height();

		width_ = std::max(width_, w);
		height_ = std::max(height_, h);
	}

	set_dim(width_, height_);
}

void preview_tileset_widget::handle_draw() const
{
	if(width_ == 0 || height_ == 0) {
		return;
	}

	const GLfloat scale = std::min(GLfloat(width())/width_, GLfloat(height())/height_);
	glPushMatrix();
	glTranslatef(GLfloat(x()), GLfloat(y()), 0);
	glScalef(scale, scale, 0.0);
	foreach(const level_tile& t, tiles_) {
		graphics::blit_queue q;
		level_object::queue_draw(q, t);
		q.do_blit();
	}
	glPopMatrix();
}

void preview_tileset_widget::set_value(const std::string& key, const variant& v)
{
	if(key == "tile_map") {
		tile_map(v).build_tiles(&tiles_);
		init();
	}
	widget::set_value(key, v);
}

variant preview_tileset_widget::get_value(const std::string& key) const
{
	return widget::get_value(key);
}

}
