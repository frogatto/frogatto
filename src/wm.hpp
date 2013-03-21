/*
	Copyright (C) 2003-2013 by David White <davewx7@gmail.com>
	
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#pragma once
#ifndef WM_HPP_INCLUDED
#define WM_HPP_INCLUDED

#if defined(USE_GLES2)

class window_manager
{
private:
	struct SDL_Surface* SDLWindow_;
#if defined(GL_ES_VERSION_2_0)
	EGLDisplay eglDisplay_;
	EGLConfig  eglConfig_;
	EGLContext eglContext_;
	EGLSurface eglSurface_;
	EGLNativeWindowType eglWindow_;
	EGLNativeDisplayType eglNativeDisplay_;
#endif
	int width_;
	int height_;

public:
	window_manager() 
		: 
#if defined(GL_ES_VERSION_2_0)
		eglDisplay_(0), eglConfig_(0), eglContext_(0), 
		eglSurface_(0),eglWindow_(0), eglNativeDisplay_(0),
#endif
		width_(0), height_(0), SDLWindow_(NULL)
	{}
	void create_window(int width, int height, int bpp, unsigned int flags);
	void destroy_window();
	void swap();
};

#endif

#endif
