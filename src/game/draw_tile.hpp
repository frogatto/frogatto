#ifndef DRAW_TILE_HPP_INCLUDED
#define DRAW_TILE_HPP_INCLUDED

#include "color_utils.hpp"
#include "geometry.hpp"
#include "texture.hpp"

namespace graphics {
class blit_queue;
}

struct level_tile;
struct hex_level_tile;

class tile_corner
{
public:
	GLshort vertex[2];
	GLfloat uv[2];
};

void queue_draw_tile(graphics::blit_queue& q, const level_tile& t);
int get_tile_corners(tile_corner* result, const graphics::texture& t, const rect& area, int tile_num, int x, int y, bool reverse);
void queue_draw_from_tilesheet(graphics::blit_queue& q, const graphics::texture& t, const rect& area, int tile_num, int x, int y, bool reverse);

bool is_tile_opaque(const graphics::texture& t, int tile_num);
bool is_tile_using_alpha_channel(const graphics::texture& t, int tile_num);
bool is_tile_solid_color(const graphics::texture& t, int tile_num, graphics::color& col);

rect get_tile_non_alpha_area(const graphics::texture& t, int tile_num);

#endif
