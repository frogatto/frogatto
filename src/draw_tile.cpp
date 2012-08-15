#include "asserts.hpp"
#include "draw_tile.hpp"
#include "level_object.hpp"
#include "raster.hpp"

#include <algorithm>
#include <iostream>

namespace {
	const int BaseTileSize = 16;
}

void queue_draw_tile(graphics::blit_queue& q, const level_tile& t)
{
	level_object::queue_draw(q, t);
}

int get_tile_corners(tile_corner* result, const graphics::texture& t, const rect& area, int tile_num, int x, int y, bool reverse)
{
	if(tile_num < 0 || area.w() <= 0 || area.h() <= 0 || area.x() < 0 || area.y() < 0) {
		return 0;
	}

	const int width = std::max<int>(t.width(), t.height());
	if (width == 0) return 0;
	
	const int xpos = BaseTileSize*(tile_num%(width/BaseTileSize)) + area.x();
	const int ypos = BaseTileSize*(tile_num/(width/BaseTileSize)) + area.y();

	//a value we subtract from the width and height of tiles when calculating
	//UV co-ordinates. This is to prevent floating point rounding errors
	//from causing us to draw slightly outside the tile. This is pretty
	//nasty stuff though, and I'm not sure of a better way to do it. :(
	const GLfloat TileEpsilon = 0.1;
	GLfloat x1 = t.translate_coord_x(GLfloat(xpos + TileEpsilon)/GLfloat(t.width()));
	GLfloat x2 = t.translate_coord_x(GLfloat(xpos+area.w() - TileEpsilon)/GLfloat(t.width()));
	const GLfloat y1 = t.translate_coord_y(GLfloat(ypos + TileEpsilon)/GLfloat(t.height()));
	const GLfloat y2 = t.translate_coord_y(GLfloat(ypos+area.h() - TileEpsilon)/GLfloat(t.height()));

	int area_x = area.x()*2;
	if(reverse) {
		std::swap(x1, x2);
		area_x = 32 - area.x()*2 - area.w()*2; // 2*BaseTileSize ?
	}

	x += area_x;
	y += area.y()*2;

	result->vertex[0] = x;
	result->vertex[1] = y;
	result->uv[0] = x1;
	result->uv[1] = y1;
	++result;

	result->vertex[0] = x;
	result->vertex[1] = y + area.h()*2;
	result->uv[0] = x1;
	result->uv[1] = y2;
	++result;

	result->vertex[0] = x + area.w()*2;
	result->vertex[1] = y;
	result->uv[0] = x2;
	result->uv[1] = y1;
	++result;

	result->vertex[0] = x + area.w()*2;
	result->vertex[1] = y + area.h()*2;
	result->uv[0] = x2;
	result->uv[1] = y2;
	++result;

	return 4;
}

void queue_draw_from_tilesheet(graphics::blit_queue& q, const graphics::texture& t, const rect& area, int tile_num, int x, int y, bool reverse)
{
	if(tile_num < 0 || area.w() <= 0 || area.h() <= 0 || area.x() < 0 || area.y() < 0) {
		return;
	}

	q.set_texture(t.get_id());

	const int width = std::max<int>(t.width(), t.height());
	const int xpos = BaseTileSize*(tile_num%(width/BaseTileSize)) + area.x();
	const int ypos = BaseTileSize*(tile_num/(width/BaseTileSize)) + area.y();

	//a value we subtract from the width and height of tiles when calculating
	//UV co-ordinates. This is to prevent floating point rounding errors
	//from causing us to draw slightly outside the tile. This is pretty
	//nasty stuff though, and I'm not sure of a better way to do it. :(
	const GLfloat TileEpsilon = 0.01;
	GLfloat x1 = t.translate_coord_x(GLfloat(xpos)/GLfloat(t.width()));
	GLfloat x2 = t.translate_coord_x(GLfloat(xpos+area.w() - TileEpsilon)/GLfloat(t.width()));
	const GLfloat y1 = t.translate_coord_y(GLfloat(ypos)/GLfloat(t.height()));
	const GLfloat y2 = t.translate_coord_y(GLfloat(ypos+area.h() - TileEpsilon)/GLfloat(t.height()));

	int area_x = area.x()*2;
	if(reverse) {
		std::swap(x1, x2);
		area_x = 32 - area.x()*2 - area.w()*2;	// 2*BaseTileSize ?
	}

	x += area_x;
	y += area.y()*2;
	q.add(x, y, x1, y1);
	q.add(x, y + area.h()*2, x1, y2);
	q.add(x + area.w()*2, y, x2, y1);
	q.add(x + area.w()*2, y + area.h()*2, x2, y2);
}

bool is_tile_opaque(const graphics::texture& t, int tile_num)
{
	const int width = std::max<int>(t.width(), t.height());
	const int xpos = BaseTileSize*(tile_num%(width/BaseTileSize));
	const int ypos = BaseTileSize*(tile_num/(width/BaseTileSize));
	for(int y = 0; y != BaseTileSize; ++y) {
		const int v = ypos + y;
		for(int x = 0; x != BaseTileSize; ++x) {
			const int u = xpos + x;
			if(t.is_alpha(u, v)) {
				return false;
			}
		}
	}
	
	return true;
}

bool is_tile_solid_color(const graphics::texture& t, int tile_num, graphics::color& col)
{
	bool first = true;
	const int width = std::max<int>(t.width(), t.height());
	const int xpos = BaseTileSize*(tile_num%(width/BaseTileSize));
	const int ypos = BaseTileSize*(tile_num/(width/BaseTileSize));
	for(int y = 0; y != BaseTileSize; ++y) {
		const int v = ypos + y;
		for(int x = 0; x != BaseTileSize; ++x) {
			const int u = xpos + x;
			const unsigned char* color = t.color_at(u, v);
			ASSERT_LOG(color != NULL, "COULD NOT FIND COLOR IN TEXTURE");
			graphics::color new_color(color[0], color[1], color[2], color[3]);
			if(new_color.a() != 0xFF) {
				return false;
			}

			if(first || col.rgba() == new_color.rgba()) {
				col = new_color;
				first = false;
			} else {
				return false;
			}
		}
	}
	
	return true;
}

rect get_tile_non_alpha_area(const graphics::texture& t, int tile_num)
{
	const int width = std::max<int>(t.width(), t.height());
	const int xpos = BaseTileSize*(tile_num%(width/BaseTileSize));
	const int ypos = BaseTileSize*(tile_num/(width/BaseTileSize));
	int top = -1, bottom = -1, left = -1, right = -1;

	for(int y = 0; y != BaseTileSize && top == -1; ++y) {
		const int v = ypos + y;
		for(int x = 0; x != BaseTileSize; ++x) {
			const int u = xpos + x;
			if(!t.is_alpha(u, v)) {
				top = y;
				break;
			}
		}
	}

	for(int y = BaseTileSize-1; y != -1 && bottom == -1; --y) {
		const int v = ypos + y;
		for(int x = 0; x != BaseTileSize; ++x) {
			const int u = xpos + x;
			if(!t.is_alpha(u, v)) {
				bottom = y + 1;
				break;
			}
		}
	}
	
	for(int x = 0; x != BaseTileSize && left == -1; ++x) {
		const int u = xpos + x;
		for(int y = 0; y != BaseTileSize; ++y) {
			const int v = ypos + y;
			if(!t.is_alpha(u, v)) {
				left = x;
				break;
			}
		}
	}

	for(int x = BaseTileSize-1; x != -1 && right == -1; --x) {
		const int u = xpos + x;
		for(int y = 0; y != BaseTileSize; ++y) {
			const int v = ypos + y;
			if(!t.is_alpha(u, v)) {
				right = x + 1;
				break;
			}
		}
	}

	if(right <= left || bottom <= top) {
		return rect();
	}

	return rect(left, top, right - left, bottom - top);
}
