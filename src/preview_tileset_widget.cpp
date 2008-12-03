#include <iostream>

#include "foreach.hpp"
#include "preview_tileset_widget.hpp"
#include "tile_map.hpp"

namespace gui {

preview_tileset_widget::preview_tileset_widget(const tile_map& tiles)
  : width_(0), height_(0)
{
	tiles.build_tiles(&tiles_);

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
	glTranslatef(x(), y(), 0);
	glScalef(scale, scale, 0.0);
	foreach(const level_tile& t, tiles_) {
		level_object::draw(t);
	}
	glPopMatrix();
}

}
