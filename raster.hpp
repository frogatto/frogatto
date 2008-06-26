
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

#include "SDL.h"

#include "texture.hpp"

namespace graphics
{

void prepare_raster();
void blit_texture(const texture& tex, int x=0, int y=0, GLfloat rotate=0.0);
void blit_texture(const texture& tex, int x, int y, int w, int h, GLfloat rotate=0.0, GLfloat x1=0.0, GLfloat y1=0.0, GLfloat x2=1.0, GLfloat y2=1.0);
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
