#include "IMG_savepng.h"
#include "asserts.hpp"
#include "draw_scene.hpp"
#include "fbo_scene.hpp"
#include "foreach.hpp"
#include "graphics.hpp"
#include "level.hpp"
#include "preferences.hpp"
#include "raster.hpp"
#include "texture.hpp"
#include "texture_frame_buffer.hpp"

#if defined(__ANDROID__)
#include <GLES/glext.h>
#endif

#if defined(TARGET_OS_HARMATTAN) || defined(TARGET_PANDORA) || defined(TARGET_TEGRA) || defined(TARGET_BLACKBERRY)
#include <EGL/egl.h>
#define glGenFramebuffersOES        preferences::glGenFramebuffersOES
#define glBindFramebufferOES        preferences::glBindFramebufferOES
#define glFramebufferTexture2DOES   preferences::glFramebufferTexture2DOES
#define glCheckFramebufferStatusOES preferences::glCheckFramebufferStatusOES
#endif

//define macros that make it easy to make the OpenGL calls in this file.
#if defined(USE_GLES2)
#define EXT_CALL(call) call
#define EXT_MACRO(macro) macro
#elif defined(TARGET_OS_HARMATTAN) || defined(TARGET_OS_IPHONE) || defined(TARGET_PANDORA) || defined(TARGET_TEGRA) || defined(TARGET_BLACKBERRY) || defined(__ANDROID__)
#define EXT_CALL(call) call##OES
#define EXT_MACRO(macro) macro##_OES
#elif defined(__APPLE__)
#define EXT_CALL(call) call##EXT
#define EXT_MACRO(macro) macro##_EXT
#else
#define EXT_CALL(call) call##EXT
#define EXT_MACRO(macro) macro##_EXT
#endif

texture_object::texture_object(const graphics::texture& texture)
  : texture_(texture)
{}

variant texture_object::get_value(const std::string& key) const
{
	return variant();
}

graphics::texture render_fbo(const rect& area, const std::vector<entity_ptr> objects)
{
	const int tex_width = graphics::texture::allows_npot() ? graphics::texture::next_power_of_2(area.w()) : area.w();
	const int tex_height = graphics::texture::allows_npot() ? graphics::texture::next_power_of_2(area.h()) : area.h();
	GLint video_framebuffer_id = 0;
	glGetIntegerv(EXT_MACRO(GL_FRAMEBUFFER_BINDING), &video_framebuffer_id);

	GLuint texture_id = 0;
	glGenTextures(1, &texture_id);
	glBindTexture(GL_TEXTURE_2D, texture_id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex_width, tex_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);

	GLuint framebuffer_id = 0;
	EXT_CALL(glGenFramebuffers)(1, &framebuffer_id);
	EXT_CALL(glBindFramebuffer)(EXT_MACRO(GL_FRAMEBUFFER), framebuffer_id);

	// attach the texture to FBO color attachment point
	EXT_CALL(glFramebufferTexture2D)(EXT_MACRO(GL_FRAMEBUFFER), EXT_MACRO(GL_COLOR_ATTACHMENT0),
                          GL_TEXTURE_2D, texture_id, 0);

	// check FBO status
	GLenum status = EXT_CALL(glCheckFramebufferStatus)(EXT_MACRO(GL_FRAMEBUFFER));
	ASSERT_NE(status, EXT_MACRO(GL_FRAMEBUFFER_UNSUPPORTED));
	ASSERT_EQ(status, EXT_MACRO(GL_FRAMEBUFFER_COMPLETE));

	glViewport(0, 0, area.w(), area.h());

	level_ptr lvl(new level("empty.cfg"));
	foreach(const entity_ptr& e, objects) {
		lvl->add_character(e);
	}

	lvl->set_boundaries(area);
	screen_position pos;
	pos.x = area.x();
	pos.y = area.y();
	{
		preferences::screen_dimension_override_scope dim_scope(area.w(), area.h(), area.w(), area.h());
		graphics::flip_draw_scope flip_draws;
		lvl->process();
		lvl->process_draw();
		render_scene(*lvl, pos);
	}

	EXT_CALL(glBindFramebuffer)(EXT_MACRO(GL_FRAMEBUFFER), video_framebuffer_id);
	glViewport(0, 0, preferences::actual_screen_width(), preferences::actual_screen_height());

	return graphics::texture(texture_id, tex_width, tex_height);
}
