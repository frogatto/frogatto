#ifndef GRAPHICS_HPP_INCLUDED
#define GRAPHICS_HPP_INCLUDED

#if TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE
#include "SDL.h"
#include "SDL_opengles.h"
#else

#if defined(TARGET_BLACKBERRY) || __MACOSX__ || __ANDROID__
#include <SDL.h>
#include <SDL_keysym.h>
#include <SDL_thread.h>
#else
#include <SDL/SDL.h>
#include <SDL/SDL_keysym.h>
#include <SDL/SDL_thread.h>
#endif

#endif

#if !TARGET_IPHONE_SIMULATOR && !TARGET_OS_IPHONE && !__MACOSX__ && !__ANDROID__
#include <SDL/SDL_mixer.h>
#endif

#if !defined(SDL_VIDEO_OPENGL_ES) && !(TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE)
#include <GL/glew.h>
#endif

#if !TARGET_IPHONE_SIMULATOR && !TARGET_OS_HARMATTAN && !TARGET_OS_IPHONE && !__MACOSX__ && !__ANDROID__
#include <SDL/SDL_ttf.h>
#endif

#if __MACOSX__ || __ANDROID__
#include <SDL_mixer.h>
#include <SDL_ttf.h>
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

#endif // GRAPHICS_HPP_INCLUDED
