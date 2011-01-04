#include <SDL.h>
#ifndef SDL_VIDEO_OPENGL_ES
#include <GL/glew.h>
#endif

#include "asserts.hpp"
#include "texture_frame_buffer.hpp"

namespace texture_frame_buffer {

namespace {
GLuint texture_id;  //ID of the texture which the frame buffer is stored in
GLuint renderbuffer_id; //renderbuffer object
GLuint framebuffer_id; //framebuffer object
}

void init()
{
	glGenTextures(1, &texture_id);
	glBindTexture(GL_TEXTURE_2D, texture_id);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE); // automatic mipmap
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width(), height(), 0,
             GL_RGBA, GL_UNSIGNED_BYTE, 0);
	glBindTexture(GL_TEXTURE_2D, 0);


	// create a renderbuffer object to store depth info
	glGenRenderbuffers(1, &renderbuffer_id);
	glBindRenderbuffer(GL_RENDERBUFFER_EXT, renderbuffer_id);
	glRenderbufferStorage(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT,
	                         width(), height());
	glBindRenderbuffer(GL_RENDERBUFFER_EXT, 0);

	// create a framebuffer object
	glGenFramebuffers(1, &framebuffer_id);
	glBindFramebuffer(GL_FRAMEBUFFER_EXT, framebuffer_id);

	// attach the texture to FBO color attachment point
	glFramebufferTexture2D(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
                          GL_TEXTURE_2D, texture_id, 0);

	// attach the renderbuffer to depth attachment point
	glFramebufferRenderbuffer(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT,
                             GL_RENDERBUFFER_EXT, renderbuffer_id);

	// check FBO status
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER_EXT);
	ASSERT_EQ(status, GL_FRAMEBUFFER_COMPLETE_EXT)

	// switch back to window-system-provided framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
}

render_scope::render_scope()
{
	glBindFramebuffer(GL_FRAMEBUFFER_EXT, framebuffer_id);
}

render_scope::~render_scope()
{
	glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);

	//explicitly generate mipmaps
	glBindTexture(GL_TEXTURE_2D, texture_id);
	glGenerateMipmapEXT(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void set_as_current_texture()
{
	glBindTexture(GL_TEXTURE_2D, texture_id);
}

} // namespace texture_frame_buffer
