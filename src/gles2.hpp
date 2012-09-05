#pragma once
#ifndef GLES2_HPP_INCLUDED
#define GLES2_HPP_INCLUDED

#include <stack>

#include "vecmath.hpp"
#include "shaders.hpp"

// tools to help conversion of GLES1.1 to 2.0

#if defined(GL_ES_VERSION_2_0)

#define GL_MODELVIEW                      0x1700
#define GL_PROJECTION                     0x1701

#define GL_FLAT                           0x1D00
#define GL_SMOOTH                         0x1D01

#define GL_CURRENT_COLOR				  0x0B00
#define GL_MODELVIEW_MATRIX               0x0BA6
#define GL_PROJECTION_MATRIX              0x0BA7

extern "C" {

void glPushMatrix();
void glPopMatrix();
void glMatrixMode(GLenum mode);
void glLoadIdentity();

void glTranslatef(GLfloat x, GLfloat y, GLfloat z);
void glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
void glScalef(GLfloat x, GLfloat y, GLfloat z);

void glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
void glColor4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha);

void glGetFloatv_1(GLenum pname, GLfloat* params);

void glShadeModel(GLenum mode);

void glOrthof(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar);

}

#else

#undef glColor4f
#define glColor4f	glColor4f_1
#undef glColor4ub
#define glColor4ub	glColor4ub_1

void glColor4f_1(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
void glColor4ub_1(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha);

#endif

namespace gles2 {
	class program;

	struct manager
	{
		manager(bool enable_tex = false);
		~manager();
		program& get();
		
		GLint pt_size;
		GLint vtx_coord;
		GLint tex_coord;
		GLint color;
	};

	void init_default_shader();
}

#endif // GLES2_HPP_INCLUDED
