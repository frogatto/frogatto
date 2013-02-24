#include "SDL_syswm.h"

#include "graphics.hpp"
#include "wm.hpp"

#if defined(USE_GLES2)

void window_manager::create_window(int width, int height, int bpp, unsigned int flags)
{
	width_ = width;
	height_ = height;
#if !defined(__native_client__)
	if(SDLWindow_ != NULL) {
		SDL_FreeSurface(SDLWindow_);
		SDLWindow_ = NULL;
		destroy_window();
	}
#if defined(GL_ES_VERSION_2_0)
	eglDisplay_ = 0;
	eglContext_ = 0;
	eglSurface_ = 0;
	eglConfig_  = 0;

#if defined(WIN32)
	//eglNativeDisplay_ = EGLNativeDisplayType(NULL);
	eglNativeDisplay_ = EGLNativeDisplayType(NULL);
#else	
	eglNativeDisplay_ = EGLNativeDisplayType(XOpenDisplay(NULL));
#endif
	eglDisplay_ = eglGetDisplay(eglNativeDisplay_);
	if(eglDisplay_ == EGL_NO_DISPLAY) {
		std::cerr << "Failed eglGetDisplay" << std::endl;
		exit(-1);
	}
	//ASSERT_LOG((eglDisplay_ = eglGetDisplay(eglNativeDisplay_)) != EGL_NO_DISPLAY, 
	//	"Failed eglGetDisplay().");

    EGLint major = 0;
    EGLint minor = 0;
	ASSERT_LOG(eglInitialize(eglDisplay_, &major, &minor), "Failed to initialise EGL.");
	ASSERT_LOG(major >= 1 && minor >= 4, "Unsupported version (must be >=1.4): " << major << "." << minor);
	ASSERT_LOG(eglBindAPI(EGL_OPENGL_ES_API), "Failed to bind OpenGL API to EGL.");

	EGLint config_attribs[7];
	config_attribs[0] = EGL_SURFACE_TYPE;
	config_attribs[1] = EGL_WINDOW_BIT;
	config_attribs[2] = EGL_RENDERABLE_TYPE;
	config_attribs[3] = EGL_OPENGL_ES2_BIT;
	config_attribs[4] = EGL_BUFFER_SIZE;
	config_attribs[5] = 16;
	config_attribs[6] = EGL_NONE;

	int configs;
	ASSERT_LOG(eglChooseConfig(eglDisplay_, config_attribs, &eglConfig_, 1, &configs) && configs == 1, 
			"eglChooseConfig failed. " << configs);

	SDLWindow_ = SDL_SetVideoMode(width_, height_, bpp, flags);
	ASSERT_LOG(SDLWindow_, "Cound not set video mode.");
	SDL_SysWMinfo sys_info;
	SDL_VERSION(&sys_info.version);
	ASSERT_LOG(SDL_GetWMInfo(&sys_info) > 0, "Unable to get the SDL window manager system information.");
#if defined(WIN32)
	eglWindow_ = EGLNativeWindowType(sys_info.window);
#else
	eglWindow_ = EGLNativeWindowType(sys_info.info.x11.window);
#endif

	eglSurface_ = eglCreateWindowSurface(eglDisplay_, eglConfig_, eglWindow_, NULL);
	int err = glGetError();
	ASSERT_LOG(err != EGL_SUCCESS, "Failed to create EGL window surface.");

	EGLint context_attribs[3];
	context_attribs[0] = EGL_CONTEXT_CLIENT_VERSION;
	context_attribs[1] = 2;
	context_attribs[2] = EGL_NONE;
	eglContext_ = eglCreateContext(eglDisplay_, eglConfig_, NULL, context_attribs);
	err = glGetError();
	ASSERT_LOG(err != EGL_SUCCESS, "Failed to create EGL context.");

	eglMakeCurrent(eglDisplay_, eglSurface_, eglSurface_, eglContext_);
	err = glGetError();
	ASSERT_LOG(err != EGL_SUCCESS, "eglMakeCurrent failed.");
#else
	flags |= SDL_OPENGL;
	SDLWindow_ = SDL_SetVideoMode(width_, height_, bpp, flags);
#endif // defined(GL_ES_VERSION_2_0)
#endif // !defined(__native_client__)
}

void window_manager::destroy_window()
{
#if !defined(__native_client__)
	if(SDLWindow_ != NULL) {
		SDL_FreeSurface(SDLWindow_);
		SDLWindow_ = NULL;
	}
#if defined(GL_ES_VERSION_2_0)
	if(eglDisplay_) {
		eglMakeCurrent(eglDisplay_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) ;
		eglTerminate(eglDisplay_);
		eglDisplay_ = 0;
		eglContext_ = 0;
		eglSurface_ = 0;
		eglConfig_  = 0;
	}
#endif // defined(GL_ES_VERSION_2_0)
#endif // !defined(__native_client__)
}

void window_manager::swap()
{
#if defined(GL_ES_VERSION_2_0)
	eglSwapBuffers(eglContext_, eglSurface_);
#endif
}

#endif // defined(GLES2)
