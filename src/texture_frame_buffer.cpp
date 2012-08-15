#include "graphics.hpp"

#include "asserts.hpp"
#include "preferences.hpp"
#include "texture.hpp"
#include "texture_frame_buffer.hpp"

#if defined(TARGET_BLACKBERRY)				// TODO: Get rid of this.
#define GL_FRAMEBUFFER_UNSUPPORTED 0x8CDD	// Krista's Hack for opengl 1 -> 2. This needs fixing badly.
#endif										// Basically, instead of fixing it, we put this line here. :(

#if defined(TARGET_OS_HARMATTAN) || defined(TARGET_PANDORA) || defined(TARGET_TEGRA) || defined(TARGET_BLACKBERRY)
#include <EGL/egl.h>
#define glGenFramebuffersOES        preferences::glGenFramebuffersOES
#define glBindFramebufferOES        preferences::glBindFramebufferOES
#define glFramebufferTexture2DOES   preferences::glFramebufferTexture2DOES
#define glCheckFramebufferStatusOES preferences::glCheckFramebufferStatusOES
#endif

//define macros that make it easy to make the OpenGL calls in this file.
#if defined(TARGET_OS_HARMATTAN) || defined(TARGET_OS_IPHONE) || defined(TARGET_PANDORA) || defined(TARGET_TEGRA) || defined(TARGET_BLACKBERRY) || defined(__ANDROID__)
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
bool supported = true;
GLuint texture_id = 0;  //ID of the texture which the frame buffer is stored in
GLuint framebuffer_id = 0; //framebuffer object
GLint video_framebuffer_id = 0; //the original frame buffer object
int frame_buffer_texture_width = 128;
int frame_buffer_texture_height = 128;
}

int width() { return frame_buffer_texture_width; }
int height() { return frame_buffer_texture_height; }

bool unsupported()
{
	return !supported;
}

void init(int buffer_width, int buffer_height)
{
	// Clear any old errors.
	glGetError();
	frame_buffer_texture_width = buffer_width;
	frame_buffer_texture_height = buffer_height;

#if defined(TARGET_OS_HARMATTAN) || defined(TARGET_PANDORA) || defined(TARGET_TEGRA) || defined(TARGET_BLACKBERRY) || defined(__ANDROID__)
	if (glGenFramebuffersOES        != NULL &&
		glBindFramebufferOES        != NULL &&
		glFramebufferTexture2DOES   != NULL &&
		glCheckFramebufferStatusOES != NULL)
	{
		supported = true;
	}
	else
	{
		fprintf(stderr, "FRAME BUFFER OBJECT NOT SUPPORTED\n");
		supported = false;
		return;
	}
#elif !TARGET_OS_IPHONE && !TARGET_IPHONE_SIMULATOR
	if(!GLEW_EXT_framebuffer_object)
    {
		fprintf(stderr, "FRAME BUFFER OBJECT NOT SUPPORTED\n");
		supported = false;
		return;
	}
#endif
	fprintf(stderr, "FRAME BUFFER OBJECT IS SUPPORTED\n");

#ifndef TARGET_TEGRA
	glGetIntegerv(EXT_MACRO(GL_FRAMEBUFFER_BINDING), &video_framebuffer_id);
#endif
	// Grab the error code first, because of the side effect in glGetError() of 
	// clearing the error code and the double call in the ASSERT_EQ() macro we lose
	// the actual error code.
	GLenum err = glGetError();
	ASSERT_EQ(err, GL_NO_ERROR);

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
	if(status == EXT_MACRO(GL_FRAMEBUFFER_UNSUPPORTED)) {
		std::cerr << "FRAME BUFFER OBJECT NOT SUPPORTED\n";
		supported = false;
		err = glGetError();
	} else {
		ASSERT_EQ(status, EXT_MACRO(GL_FRAMEBUFFER_COMPLETE));
	}

	// switch back to window-system-provided framebuffer
	EXT_CALL(glBindFramebuffer)(EXT_MACRO(GL_FRAMEBUFFER), video_framebuffer_id);

	err = glGetError();
	ASSERT_EQ(err, GL_NO_ERROR);
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
	graphics::texture::set_current_texture(texture_id);
}

} // namespace texture_frame_buffer
