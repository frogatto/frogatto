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
	class fixed_program : public program
	{
	public:
		fixed_program();
		explicit fixed_program(const std::string& name, const shader& vs, const shader& fs);
		virtual ~fixed_program()
		{}
		virtual void vertex_array(GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* ptr);
		virtual void texture_array(GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* ptr);
		virtual void color_array(GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* ptr);

		virtual void set_fixed_attributes(const variant& node);
		static void add_shader(const std::string& program_name, 
			const shader& v_shader, 
			const shader& f_shader, 
			const variant& prog);
	private:
		typedef std::vector<GLfloat> gl_attribute;
		typedef boost::shared_ptr<gl_attribute> attribute_ptr;
		typedef boost::shared_ptr<const gl_attribute> const_attribute_ptr;
		std::map<GLint, const_attribute_ptr> saved_attributes;

		// Same cached variables to speed things up.
		GLint vtx_coord_;
		GLint tex_coord_[2];
		GLint col_coord_;
	};

	typedef boost::intrusive_ptr<fixed_program> fixed_program_ptr;
	typedef boost::intrusive_ptr<const fixed_program> const_fixed_program_ptr;

	shader_ptr get_tex_shader();
	shader_ptr get_texcol_shader();
	shader_ptr get_simple_shader();
	shader_ptr get_simple_col_shader();
	
	shader_ptr active_shader();

	struct manager
	{
		manager(shader_ptr shader = NULL);
		~manager();
	};

	void init_default_shader();
	const mat4& get_mvp_matrix();
	GLfloat get_alpha();
	GLfloat* get_color();
}

#endif // GLES2_HPP_INCLUDED
