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
