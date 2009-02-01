
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

#include "asserts.hpp"
#include "raster.hpp"
#include "raster_distortion.hpp"

#include <boost/shared_array.hpp>
#include <iostream>
#include <cmath>

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

namespace {
rect draw_detection_rect_;
char* draw_detection_buf_;

std::vector<const raster_distortion*> distortions_;
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

namespace {
void blit_texture_internal(const texture& tex, int x, int y, int w, int h, GLfloat rotate, GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)
{
	if(!tex.valid()) {
		return;
	}

	const int orig_w = w;
	const int orig_h = h;

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

	if(draw_detection_buf_) {
		rect draw_rect(x, y, std::abs(orig_w), std::abs(orig_h));
		if(rects_intersect(draw_rect, draw_detection_rect_)) {
			rect r = intersection_rect(draw_rect, draw_detection_rect_);
			for(int ypos = r.y(); ypos != r.y2(); ++ypos) {
				for(int xpos = r.x(); xpos != r.x2(); ++xpos) {
					const GLfloat u = (GLfloat(draw_rect.x2() - xpos)*x1 + GLfloat(xpos - draw_rect.x())*x2)/GLfloat(draw_rect.w());
					const GLfloat v = (GLfloat(draw_rect.y2() - ypos)*y1 + GLfloat(ypos - draw_rect.y())*y2)/GLfloat(draw_rect.h());
					const int texture_x = u*tex.width();
					const int texture_y = v*tex.height();
					ASSERT_GE(texture_x, 0);
					ASSERT_GE(texture_y, 0);
					ASSERT_LOG(texture_x < tex.width(), texture_x << " < " << tex.width() << " " << r.x() << " " << r.x2() << " " << xpos << " x: " << x1 << " x2: " << x2 << " u: " << u << "\n");
					ASSERT_LT(texture_x, tex.width());
					ASSERT_LT(texture_y, tex.height());
					const bool alpha = tex.is_alpha(texture_x, texture_y);
					if(!alpha) {
						const int buf_x = xpos - draw_detection_rect_.x();
						const int buf_y = ypos - draw_detection_rect_.y();
						const int buf_index = buf_y*draw_detection_rect_.w() + buf_x;
						ASSERT_LOG(buf_index >= 0, xpos << ", " << ypos << " -> " << buf_x << ", " << buf_y << " -> " << buf_index << " in " << draw_detection_rect_ << "\n");
						ASSERT_GE(buf_index, 0);
						ASSERT_LT(buf_index, draw_detection_rect_.w()*draw_detection_rect_.h());
						draw_detection_buf_[buf_index] = true;
					}
				}
			}
		}
	}
}

void blit_texture_with_distortion(const texture& tex, int x, int y, int w, int h, GLfloat rotate, GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2, const raster_distortion& distort)
{
	const rect& area = distort.area();
	if(x < area.x()) {
		const int new_x = area.x();
		const GLfloat new_x1 = (x1*(x + w - new_x) + x2*(new_x - x))/w;

		blit_texture(tex, x, y, new_x - x, h, rotate, x1, y1, new_x1, y2);

		x1 = new_x1;
		w -= new_x - x;
		x = new_x;
	}

	if(y < area.y()) {
		const int new_y = area.y();
		const GLfloat new_y1 = (y1*(y + h - new_y) + y2*(new_y - y))/h;

		blit_texture(tex, x, y, w, new_y - y, rotate, x1, y1, x2, new_y1);

		y1 = new_y1;
		h -= new_y - y;
		y = new_y;
	}

	if(x + w > area.x2()) {
		const int new_w = area.x2() - x;
		const int new_xpos = x + new_w;
		const GLfloat new_x2 = (x1*(x + w - new_xpos) + x2*(new_xpos - x))/w;

		blit_texture(tex, new_xpos, y, x + w - new_xpos, h, rotate, new_x2, y1, x2, y2);

		x2 = new_x2;
		w = new_w;
	}

	if(y + h > area.y2()) {
		const int new_h = area.y2() - y;
		const int new_ypos = y + new_h;
		const GLfloat new_y2 = (y1*(y + h - new_ypos) + y2*(new_ypos - y))/h;

		blit_texture(tex, x, new_ypos, w, y + h - new_ypos, rotate, x1, new_y2, x2, y2);

		y2 = new_y2;
		h = new_h;
	}

	const int xdiff = distort.granularity_x();
	const int ydiff = distort.granularity_y();
	for(int xpos = 0; xpos < w; xpos += xdiff) {
		const int xbegin = x + xpos;
		const int xend = std::min<int>(x + w, xbegin + xdiff);

		const GLfloat u1 = (x1*(x+w - xbegin) + x2*(xbegin - x))/w;
		const GLfloat u2 = (x1*(x+w - xend) + x2*(xend - x))/w;
		for(int ypos = 0; ypos < h; ypos += ydiff) {
			const int ybegin = y + ypos;
			const int yend = std::min<int>(y + h, ybegin + ydiff);

			const GLfloat v1 = (y1*(y+h - ybegin) + y2*(ybegin - y))/h;
			const GLfloat v2 = (y1*(y+h - yend) + y2*(yend - y))/h;

			int xbegin_distort = xbegin;
			int ybegin_distort = ybegin;
			int xend_distort = xend;
			int yend_distort = yend;
			distort.distort_point(&xbegin_distort, &ybegin_distort);
			distort.distort_point(&xend_distort, &yend_distort);
			blit_texture_internal(tex, xbegin_distort, ybegin_distort,
			                           xend_distort - xbegin_distort,
			                           yend_distort - ybegin_distort,
			                           rotate, u1, v1, u2, v2);
		}
	}
}

}  // namespace

void blit_texture(const texture& tex, int x, int y, int w, int h, GLfloat rotate, GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)
{
	if(w < 0) {
		std::swap(x1, x2);
		w *= -1;
	}

	if(h < 0) {
		std::swap(y1, y2);
		h *= -1;
	}

	for(std::vector<const raster_distortion*>::const_iterator i = distortions_.begin(); i != distortions_.end(); ++i) {
		const raster_distortion& distort = **i;
		if(rects_intersect(rect(x, y, w, h), distort.area())) {
			blit_texture_with_distortion(tex, x, y, w, h, rotate, x1, y1, x2, y2, distort);
			return;
		}
	}
	blit_texture_internal(tex, x, y, w, h, rotate, x1, y1, x2, y2);
}

void set_draw_detection_rect(const rect& rect, char* buf)
{
	draw_detection_rect_ = rect;
	draw_detection_buf_ = buf;
}

void clear_draw_detection_rect()
{
	draw_detection_buf_ = NULL;
}

void add_raster_distortion(const raster_distortion* distortion)
{
	distortions_.push_back(distortion);
}

void remove_raster_distortion(const raster_distortion* distortion)
{
	distortions_.erase(std::remove(distortions_.begin(), distortions_.end(), distortion), distortions_.end());
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
