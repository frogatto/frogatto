
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#include <GL/gl.h>
#include <GL/glu.h>
#include <SDL.h>

#include "raster.hpp"

#include <boost/shared_array.hpp>
#include <iostream>

namespace graphics
{

/* unavoidable global variable to store global clip
   rectangle changes */
std::vector<boost::shared_array<GLint> > clip_rectangles;

void prepare_raster()
{
	const SDL_Surface* fb = SDL_GetVideoSurface();
	if(fb == NULL) {
		return;
	}

	glViewport(0,0,fb->w,fb->h);
	glClearColor(0.0,0.0,0.0,0.0);
	glClear(GL_COLOR_BUFFER_BIT);
	glShadeModel(GL_FLAT);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0,screen_width(),screen_height(),0,-1.0,1.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glDisable(GL_LIGHT0);

	glColor4f(1.0, 1.0, 1.0, 1.0);
}

void blit_texture(const texture& tex, int x, int y, GLfloat rotate)
{
	if(!tex.valid()) {
		return;
	}

	int h = tex.height();
	int w = tex.width();
	const int w_odd = w % 2;
	const int h_odd = h % 2;
	h /= 2;
	w /= 2;

	glPushMatrix();

	glTranslatef(x+w,y+h,0.0);
	glRotatef(rotate,0.0,0.0,1.0);

	tex.set_as_current_texture();

	glBegin(GL_QUADS);
	graphics::texture::set_coord(0.0,0.0);
	glVertex3i(-w,-h,0);
	graphics::texture::set_coord(0.0,1.0);
	glVertex3i(-w,h+h_odd,0);
	graphics::texture::set_coord(1.0,1.0);
	glVertex3i(w+w_odd,h+h_odd,0);
	graphics::texture::set_coord(1.0,0.0);
	glVertex3i(w+w_odd,-h,0);
	glEnd();

	glPopMatrix();
}

void blit_texture(const texture& tex, int x, int y, int w, int h, GLfloat rotate, GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)
{
	if(!tex.valid()) {
		return;
	}

	const int w_odd = w % 2;
	const int h_odd = h % 2;

	w /= 2;
	h /= 2;
	glPushMatrix();
	tex.set_as_current_texture();
	glTranslatef(x+abs(w),y+abs(h),0.0);
	glRotatef(rotate,0.0,0.0,1.0);
	glBegin(GL_QUADS);
	graphics::texture::set_coord(x1,y1);
	glVertex3i(-w,-h,0);
	graphics::texture::set_coord(x1,y2);
	glVertex3i(-w,h+h_odd,0);
	graphics::texture::set_coord(x2,y2);
	glVertex3i(w+w_odd,h+h_odd,0);
	graphics::texture::set_coord(x2,y1);
	glVertex3i(w+w_odd,-h,0);
	glEnd();
	glPopMatrix();
}

void draw_rect(const SDL_Rect& r, const SDL_Color& color,
               unsigned char alpha)
{
	glDisable(GL_TEXTURE_2D);
	glColor4ub(color.r,color.g,color.b,alpha);
	glRecti(r.x,r.y,r.x+r.w,r.y+r.h);
	glColor4ub(255, 255, 255, 255);
	glEnable(GL_TEXTURE_2D);
}

void draw_hollow_rect(const SDL_Rect& r, const SDL_Color& color,
		      unsigned char alpha) {

	glDisable(GL_TEXTURE_2D);
	glBegin(GL_LINE_LOOP);
	glColor4ub(color.r, color.g, color.b, alpha);
	glVertex3f( r.x, r.y, 0);
	glVertex3f( r.x + r.w, r.y, 0);
	glVertex3f( r.x + r.w, r.y + r.h, 0);
	glVertex3f( r.x, r.y + r.h, 0);
	glColor4ub(255, 255, 255, 255);
	glEnd();
	glEnable(GL_TEXTURE_2D);
}

void coords_to_screen(GLdouble sx, GLdouble sy, GLdouble sz,
		      GLdouble* dx, GLdouble* dy, GLdouble* dz) {
	GLdouble model[16], proj[16];
	GLint view[4];

	glGetDoublev(GL_MODELVIEW_MATRIX, model);
	glGetDoublev(GL_PROJECTION_MATRIX, proj);
	glGetIntegerv(GL_VIEWPORT, view);

	gluProject(sx, sy, sz, model, proj, view, dx, dy, dz);
}

void push_clip(const SDL_Rect& r)
{
	const bool was_enabled_clip = glIsEnabled(GL_SCISSOR_TEST);
	bool should_enable_clip = true;

	GLdouble x, y ,z;
	coords_to_screen(0, graphics::screen_height(), 0, &x, &y, &z);

	SDL_Rect s = r;
	s.x += static_cast<Sint16>(x);
	s.y -= static_cast<Sint16>(y);

	if(s.x == 0 && s.y == 0 && s.w == screen_width() && s.h == screen_height()) {
		should_enable_clip = false;
	}


	boost::shared_array<GLint> box(new GLint[4]);

	if(was_enabled_clip) {
		glGetIntegerv(GL_SCISSOR_BOX, box.get());
	} else {
		box[0] = 0;
		box[1] = 0;
		box[2] = screen_width();
		box[3] = screen_height();
	}
	clip_rectangles.push_back(box);

	if(should_enable_clip) {
		if(!was_enabled_clip) {
			glEnable(GL_SCISSOR_TEST);
		}
		glScissor(s.x, screen_height() - (s.y + s.h), s.w, s.h);
	} else if(was_enabled_clip) {
		glDisable(GL_SCISSOR_TEST);
	}
}
void pop_clip() {
	const bool was_enabled_clip = glIsEnabled(GL_SCISSOR_TEST);
	bool should_enable_clip = false;
	boost::shared_array<GLint> box;

	if(!clip_rectangles.empty()) {
		box = *clip_rectangles.rbegin();
		clip_rectangles.pop_back();

		if(box[0] != 0 || box[1] != 0 || box[2] != screen_width() || box[3] != screen_height()) {
			should_enable_clip = true;
		}
	}

	if(should_enable_clip) {
		if(!was_enabled_clip) {
			glEnable(GL_SCISSOR_TEST);
		}
		glScissor(box[0], box[1], box[2], box[3]);
	} else if(was_enabled_clip) {
		glDisable(GL_SCISSOR_TEST);
	}
}

int screen_width()
{
	return 800;
	/*
	SDL_Surface* surf = SDL_GetVideoSurface();
	if(surf) {
		return SDL_GetVideoSurface()->w;
	} else {
		return 1024;
	}*/
}

int screen_height()
{
	return 600;
	/*
	SDL_Surface* surf = SDL_GetVideoSurface();
	if(surf) {
		return SDL_GetVideoSurface()->h;
	} else {
		return 768;
	}*/
}

const SDL_Color& color_black()
{
	static SDL_Color res = {0,0,0,0};
	return res;
}

const SDL_Color& color_white()
{
	static SDL_Color res = {0xFF,0xFF,0xFF,0};
	return res;
}

const SDL_Color& color_red()
{
	static SDL_Color res = {0xFF,0,0,0};
	return res;
}

const SDL_Color& color_green()
{
	static SDL_Color res = {0,0xFF,0,0};
	return res;
}

const SDL_Color& color_blue()
{
	static SDL_Color res = {0,0,0xFF,0};
	return res;
}

const SDL_Color& color_yellow()
{
	static SDL_Color res = {0xFF,0xFF,0,0};
	return res;
}

}
