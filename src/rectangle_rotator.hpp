#ifndef RECTANGLE_ROTATOR_HPP_INCLUDED
#define RECTANGLE_ROTATOR_HPP_INCLUDED

#include "geometry.hpp"

void rotate_rect(GLshort center_x, GLshort center_y, float rotation, GLshort* rect_vertexes);
void rotate_rect(const rect& r, GLfloat angle, GLshort* output);
point rotate_point_around_origin_with_offset(int x1, int y1, float alpha, int u1, int v1);
point rotate_point_around_origin(int x1, int y1, float alpha);

#endif
