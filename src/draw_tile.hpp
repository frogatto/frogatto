#ifndef DRAW_TILE_HPP_INCLUDED
#define DRAW_TILE_HPP_INCLUDED

#include "color_utils.hpp"
#include "level_object.hpp"
#include "texture.hpp"

namespace graphics {
class blit_queue;
}

void queue_draw_tile(graphics::blit_queue& q, const level_tile& t);
void draw_tile(const level_tile& t);
void queue_draw_from_tilesheet(graphics::blit_queue& q, const graphics::texture& t, int tile_num, int x, int y, bool reverse);
void draw_from_tilesheet(const graphics::texture& t, int tile_num, int x, int y, bool reverse, int rotate=0);

bool is_tile_opaque(const graphics::texture& t, int tile_num);
bool is_tile_solid_color(const graphics::texture& t, int tile_num, graphics::color& col);

#endif
