#include <SDL.h>
#ifndef SDL_VIDEO_OPENGL_ES
#include <GL/glew.h>
#endif

#include "asserts.hpp"
#include "preferences.hpp"
#include "texture_frame_buffer.hpp"

//define macros that make it easy to make the OpenGL calls in this file.
#if TARGET_OS_IPHONE
#define EXT_CALL(call) call##OES
#define EXT_MACRO(macro) macro##_OES
#elif defined(__APPLE__)
#define EXT_CALL(call) call##EXT
#define EXT_MACRO(macro) macro##_EXT
#else
#define EXT_CALL(call) call##EXT
#define EXT_MACRO(macro) macro##_EXT
#endif

namespace texture_frame_buffer {

namespace {
GLuint texture_id = 0;  //ID of the texture which the frame buffer is stored in
GLuint framebuffer_id = 0; //framebuffer object
GLint video_framebuffer_id = 0; //the original frame buffer object
}

void init()
{
	glGetIntegerv(EXT_MACRO(GL_FRAMEBUFFER_BINDING), &video_framebuffer_id);

	ASSERT_EQ(glGetError(), GL_NO_ERROR);
	glGenTextures(1, &texture_id);
	glBindTexture(GL_TEXTURE_2D, texture_id);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width(), height(), 0,
             GL_RGBA, GL_UNSIGNED_BYTE, 0);
	glBindTexture(GL_TEXTURE_2D, 0);


	// create a framebuffer object
	EXT_CALL(glGenFramebuffers)(1, &framebuffer_id);
	EXT_CALL(glBindFramebuffer)(EXT_MACRO(GL_FRAMEBUFFER), framebuffer_id);

	// attach the texture to FBO color attachment point
	EXT_CALL(glFramebufferTexture2D)(EXT_MACRO(GL_FRAMEBUFFER), EXT_MACRO(GL_COLOR_ATTACHMENT0),
                          GL_TEXTURE_2D, texture_id, 0);

	// check FBO status
	GLenum status = EXT_CALL(glCheckFramebufferStatus)(EXT_MACRO(GL_FRAMEBUFFER));
	ASSERT_EQ(status, EXT_MACRO(GL_FRAMEBUFFER_COMPLETE));

	// switch back to window-system-provided framebuffer
	EXT_CALL(glBindFramebuffer)(EXT_MACRO(GL_FRAMEBUFFER), video_framebuffer_id);

	ASSERT_EQ(glGetError(), GL_NO_ERROR);
}

render_scope::render_scope()
{
	EXT_CALL(glBindFramebuffer)(EXT_MACRO(GL_FRAMEBUFFER), framebuffer_id);
	glViewport(0, 0, width(), height());
}

render_scope::~render_scope()
{
	EXT_CALL(glBindFramebuffer)(EXT_MACRO(GL_FRAMEBUFFER), video_framebuffer_id);
	glViewport(0, 0, preferences::actual_screen_width(), preferences::actual_screen_height());
}

void set_as_current_texture()
{
	glBindTexture(GL_TEXTURE_2D, texture_id);
}

} // namespace texture_frame_buffer
