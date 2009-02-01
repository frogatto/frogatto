
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef RASTER_HPP_INCLUDED
#define RASTER_HPP_INCLUDED

#include <vector>

#include "SDL.h"

#include "geometry.hpp"
#include "texture.hpp"

namespace graphics
{

void prepare_raster();
void blit_texture(const texture& tex, int x=0, int y=0, GLfloat rotate=0.0);

//Function to blit a texture to the screen. Parameters:
//x, y: target on-screen location.
//w, h: dimensions of the on-screen area that will be filled by the
//blit.
//rotate: the number of degrees to rotate by when blitting
//x1, y1, x2, y2: the area of the texture to blit onto the screen. The
//defaults are to blit the entire texture. Note that these values can be
// < 0.0 or > 1.0 and the texture will wrap, but *only* if the texture's
//dimensions are powers of two. Otherwise they must be in the range [0,1]
void blit_texture(const texture& tex, int x, int y, int w, int h, GLfloat rotate=0.0, GLfloat x1=0.0, GLfloat y1=0.0, GLfloat x2=1.0, GLfloat y2=1.0);

//function which sets a rectangle where we want to detect if pixels are written.
//buf must point to a buffer with a size of rect.w*rect.h. Whenever a pixel
//is blitted within rect, the corresponding pixel in buf will be set. buf
//must remain valid until another call to set_draw_detection_rect() or a
//call to clear_draw_detection_rect().
void set_draw_detection_rect(const rect& rect, char* buf);
void clear_draw_detection_rect();

class raster_distortion;
void add_raster_distortion(const raster_distortion* distortion);
void remove_raster_distortion(const raster_distortion* distortion);

void draw_rect(const SDL_Rect& rect, const SDL_Color& color,
               unsigned char alpha=0xFF);
void draw_hollow_rect(const SDL_Rect& rect, const SDL_Color& color,
               unsigned char alpha=0xFF);
int screen_width();
int screen_height();

void coords_to_screen(GLdouble sx, GLdouble sy, GLdouble sz,
		      GLdouble* dx, GLdouble* dy, GLdouble* dz);
void push_clip(const SDL_Rect& rect);
void pop_clip();

const SDL_Color& color_black();
const SDL_Color& color_white();
const SDL_Color& color_red();
const SDL_Color& color_green();
const SDL_Color& color_blue();
const SDL_Color& color_yellow();

}

#endif
