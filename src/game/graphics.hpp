#ifndef GRAPHICS_HPP_INCLUDED
#define GRAPHICS_HPP_INCLUDED

#if TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE
#include "SDL.h"
#include "SDL_opengles.h"
#endif

#include "SDL.h"
#include "SDL_keysym.h"
#include "SDL_thread.h"

#if defined(__ANDROID__)
#include "SDL_screenkeyboard.h"
#endif

#if !TARGET_IPHONE_SIMULATOR && !TARGET_OS_HARMATTAN && !TARGET_OS_IPHONE
#include "SDL_ttf.h"
#include "SDL_mixer.h"
#endif

#ifdef USE_GLES2

#if defined(WIN32) || defined(__linux__)
#include <GL/glew.h>
#else
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#endif

#include "gles2.hpp"
#include "wm.hpp"

#if defined(GL_ES_VERSION_2_0)
extern window_manager wm;
#undef SDL_GL_SwapBuffers
#define SDL_GL_SwapBuffers()	do{wm.swap();}while(0)
#endif

#else

#if !defined(SDL_VIDEO_OPENGL_ES) && !(TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE) && !defined(__ANDROID__) && !defined(__native_client__)
#include <GL/glew.h>
#endif

#if defined(__native_client__)
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
#endif

#if defined(TARGET_OS_HARMATTAN) || defined(TARGET_PANDORA) || defined(TARGET_TEGRA) || defined(TARGET_BLACKBERRY)
#include <GLES/gl.h>
#ifdef TARGET_PANDORA
#include <GLES/glues.h>
#endif
#include <GLES/glext.h>
#else
#if defined(__ANDROID__)
#include <GLES/gl.h>
#include <GLES/glext.h>
#include <GLES/glplatform.h>
#else
#if defined( _WINDOWS )
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#include <GL/gl.h>
#include <GL/glu.h>
#endif
#endif

#if defined(__APPLE__) && !(TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE)
#include <OpenGL/OpenGL.h>
#endif

#endif

#endif // GRAPHICS_HPP_INCLUDED
