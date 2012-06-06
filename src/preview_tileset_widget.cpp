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
