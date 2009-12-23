#include "draw_tile.hpp"
#include "raster.hpp"

#include <algorithm>
#include <iostream>

void queue_draw_tile(graphics::blit_queue& q, const level_tile& t)
{
	level_object::queue_draw(q, t);
}

void draw_tile(const level_tile& t)
{
	level_object::draw(t);
}

void queue_draw_from_tilesheet(graphics::blit_queue& q, const graphics::texture& t, int tile_num, int x, int y, bool reverse)
{
	if(tile_num < 0) {
		return;
	}

	q.set_texture(t.get_id());

	const int width = std::max<int>(t.width(), t.height());
	const int xpos = 16*(tile_num%(width/16));
	const int ypos = 16*(tile_num/(width/16));

	GLfloat x1 = t.translate_coord_x(GLfloat(xpos)/GLfloat(t.width()));
	GLfloat x2 = t.translate_coord_x(GLfloat(xpos+16)/GLfloat(t.width()));
	const GLfloat y1 = t.translate_coord_y(GLfloat(ypos)/GLfloat(t.height()));
	const GLfloat y2 = t.translate_coord_y(GLfloat(ypos+16)/GLfloat(t.height()));

	if(reverse) {
		std::swap(x1, x2);
	}

	q.add(x, y, x1, y1);
	q.add(x, y + 32, x1, y2);
	q.add(x + 32, y, x2, y1);
	q.add(x + 32, y + 32, x2, y2);
}

void draw_from_tilesheet(const graphics::texture& t, int tile_num, int x, int y, bool reverse, int rotate)
{
	if(tile_num < 0) {
		return;
	}

	const int width = std::max<int>(t.width(), t.height());
	const int xpos = 16*(tile_num%(width/16));
	const int ypos = 16*(tile_num/(width/16));

	GLfloat x1 = GLfloat(xpos)/GLfloat(t.width());
	GLfloat x2 = GLfloat(xpos+16)/GLfloat(t.width());
	const GLfloat y1 = GLfloat(ypos)/GLfloat(t.height());
	const GLfloat y2 = GLfloat(ypos+16)/GLfloat(t.height());

	if(reverse) {
		std::swap(x1, x2);
	}

	queue_blit_texture(t, x, y, 32, 32, x1, y1, x2, y2);

//	graphics::blit_texture(t, x, y, 32, 32, rotate, x1, y1, x2, y2);
}
