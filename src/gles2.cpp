#include "asserts.hpp"
#include "filesystem.hpp"
#include "gles2.hpp"

namespace {
	typedef std::stack<mat4> projection_mat_stack;
	typedef std::stack<mat4> modelview_mat_stack;

	GLenum matrix_mode = GL_PROJECTION;
	projection_mat_stack p_mat_stack;
	modelview_mat_stack mv_mat_stack;

	// Current project/modelview matricies
	mat4 proj_matrix = mat4::identity();
	mat4 modelview_matrix = mat4::identity();

	float colors[4] = {1.0f, 1.0f, 1.0f, 1.0f};

	GLenum shade_model = GL_FLAT;
}

#if defined(GL_ES_VERSION_2_0)

void glMatrixMode(GLenum mode)
{
	ASSERT_LOG(mode == GL_MODELVIEW || mode == GL_PROJECTION, "Unrecognised matrix mode: " << mode)
	matrix_mode = mode;
}

void glPushMatrix()
{
	if(matrix_mode == GL_MODELVIEW) {
		mv_mat_stack.push(modelview_matrix);
	} else if(matrix_mode == GL_PROJECTION) {
		p_mat_stack.push(proj_matrix);
	}
}

void glPopMatrix()
{
	if(matrix_mode == GL_MODELVIEW) {
		modelview_matrix = mv_mat_stack.top();
		mv_mat_stack.pop();
	} else if(matrix_mode == GL_PROJECTION) {
		proj_matrix = p_mat_stack.top();
		p_mat_stack.pop();
	}
}

void glLoadIdentity()
{
	if(matrix_mode == GL_MODELVIEW) {
		modelview_matrix = mat4::identity();
	} else if(matrix_mode == GL_PROJECTION) {
		proj_matrix = mat4::identity();
	}
}

void glTranslatef(GLfloat x, GLfloat y, GLfloat z)
{
	if(matrix_mode == GL_MODELVIEW) {
		modelview_matrix.translate(x, y, z);
	} else if(matrix_mode == GL_PROJECTION) {
		proj_matrix.translate(x, y, z);
	}
}

void glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
	if(matrix_mode == GL_MODELVIEW) {
		modelview_matrix.rotate(angle, x, y, z);
	} else if(matrix_mode == GL_PROJECTION) {
		proj_matrix.rotate(angle, x, y, z);
	}
}

void glScalef (GLfloat x, GLfloat y, GLfloat z)
{
	if(matrix_mode == GL_MODELVIEW) {
		modelview_matrix.scale(x, y, z);
	} else if(matrix_mode == GL_PROJECTION) {
		proj_matrix.scale(x, y, z);
	}
}

void glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
	colors[0] = red;
	colors[1] = green;
	colors[2] = blue;
	colors[3] = alpha;
}

void glColor4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha)
{
	colors[0] = float(red)/255.0f;
	colors[1] = float(green)/255.0f;
	colors[2] = float(blue)/255.0f;
	colors[3] = float(alpha)/255.0f;
}

void glGetFloatv_1(GLenum pname, GLfloat* params)
{
	ASSERT_LOG(params != NULL, "params must not be null");
	if(pname == GL_CURRENT_COLOR) {
		memcpy(params, colors, sizeof(colors));
	} else if(pname == GL_MODELVIEW_MATRIX) {
		memcpy(params, &modelview_matrix.x.x, sizeof(modelview_matrix));
	} else if(pname == GL_PROJECTION_MATRIX) {
		memcpy(params, &proj_matrix.x.x, sizeof(proj_matrix));
	} else {
		ASSERT_LOG(false, "Unsupported mode in the call: " << pname);
	}
}

void glShadeModel(GLenum mode)
{
	ASSERT_LOG(mode == GL_FLAT || mode == GL_SMOOTH, "Unrecognised shade mode: " << mode)
	shade_model = mode;
}

void glOrthof(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar)
{
    float dx = right - left;
    float dy = top - bottom;
    float dz = zFar - zNear;
	mat4 ortho = mat4::identity();

    if((dx == 0.0f) || (dy == 0.0f) || (dz == 0.0f)) {
        return;
	}

	ortho.x.x = 2.0f / dx;
	ortho.w.x = -(right + left) / dx;
	ortho.y.y = 2.0f / dy;
    ortho.w.y = -(top + bottom) / dy;
    ortho.z.z = -2.0f / dz;
    ortho.w.z = -(zNear + zFar) / dz;

	if(matrix_mode == GL_MODELVIEW) {
		modelview_matrix = ortho * modelview_matrix;
	} else if(matrix_mode == GL_PROJECTION) {
		proj_matrix = ortho * proj_matrix;
	}
}
#else

void glColor4f_1(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
	colors[0] = red;
	colors[1] = green;
	colors[2] = blue;
	colors[3] = alpha;
}

void glColor4ub_1(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha)
{
	colors[0] = float(red)/255.0f;
	colors[1] = float(green)/255.0f;
	colors[2] = float(blue)/255.0f;
	colors[3] = float(alpha)/255.0f;
}

#endif

/////////////////////////////////////////////////////

namespace {
	const std::string fs1 = 
		//"precision mediump float;\n"
		"varying vec4 v_color;\n"
		"uniform vec4 u_color;\n"
		"void main()\n"
		"{\n"
		//"	gl_FragColor = v_color;\n"
		"	gl_FragColor = u_color;\n"
		"}\n";
	const std::string vs1 = 
		"uniform mat4 mvp_matrix;\n"
		"attribute vec4 a_position;\n"
		"attribute vec4 a_color;\n"
		"uniform float a_point_size;\n"
		"varying vec4 v_color;\n"
		"void main()\n"
		"{\n"
		"	v_color = a_color;\n"
		"	gl_PointSize = a_point_size;\n"
		"	gl_Position = mvp_matrix * a_position;\n"
		"}\n";

	const std::string fs_tex = 
		//"precision mediump float;\n"
		"uniform sampler2D s_tex_map;\n"
		"varying vec2 v_texcoord;\n"
		"uniform float u_alpha;\n"
		"void main()\n"
		"{\n"
		"	gl_FragColor = mix(texture2D(s_tex_map, v_texcoord), vec4(1.0,1.0,1.0,0.0), 1.0-u_alpha);\n"
		"}\n";
	const std::string vs_tex = 
		"uniform mat4 mvp_matrix;\n"
		"attribute vec4 a_position;\n"
		"attribute vec4 a_color;\n"
		"attribute vec2 a_texcoord;\n"
		"varying vec2 v_texcoord;\n"
		"void main()\n"
		"{\n"
		"	v_texcoord = a_texcoord;\n"
		"	gl_Position = mvp_matrix * a_position;\n"
		"}\n";

	gles2::program tex_shader_program;
	gles2::program simple_shader_program;
}


mat4 gles2_prepare_draw()
{
#if defined(GL_ES_VERSION_2_0)
	return proj_matrix * modelview_matrix;
#else
	mat4 projection = mat4::identity();
	mat4 modelview_matrix = mat4::identity();
	glGetFloatv(GL_MODELVIEW_MATRIX, &modelview_matrix.x.x);
	glGetFloatv(GL_PROJECTION_MATRIX, &projection.x.x);
	return projection * modelview_matrix;
#endif
}

namespace gles2 {
	void init_default_shader()
	{
		gles2::shader v1(GL_VERTEX_SHADER, "simple_vertex_shader", vs1);
		gles2::shader f1(GL_FRAGMENT_SHADER, "simple_fragment_shader", fs1);
		simple_shader_program.init(v1, f1);

		gles2::shader v_tex(GL_VERTEX_SHADER, "tex_vertex_shader", vs_tex);
		gles2::shader f_tex(GL_FRAGMENT_SHADER, "tex_fragment_shader", fs_tex);
		tex_shader_program.init(v_tex, f_tex);

		glUniform1i(tex_shader_program.get_uniform("s_tex_map"), 0);

		matrix_mode = GL_PROJECTION;
		p_mat_stack.empty();
		mv_mat_stack.empty();
		proj_matrix = mat4::identity();
		modelview_matrix = mat4::identity();
		colors[0] = 1.0f;
		colors[1] = 1.0f;
		colors[2] = 1.0f;
		colors[2] = 1.0f;
		glActiveTexture(GL_TEXTURE0);
	}

	manager::manager(bool enable_tex)
	{
		mat4 mvp = gles2_prepare_draw();
		if(enable_tex) {
			vtx_coord = tex_shader_program.get_attribute("a_position");
			tex_coord = tex_shader_program.get_attribute("a_texcoord");
			//glVertexAttrib4fv(tex_shader_program.get_attribute("a_color"), colors);
			glUseProgram(tex_shader_program.get());
			glUniform1f(tex_shader_program.get_uniform("u_alpha"), colors[3]);
			glUniformMatrix4fv(tex_shader_program.get_uniform("mvp_matrix"), 
				1, 
				GL_FALSE, 
				(GLfloat*) &mvp.x.x );
		} else {
			vtx_coord = simple_shader_program.get_attribute("a_position");
			color     = simple_shader_program.get_attribute("a_color"); 
			pt_size   = simple_shader_program.get_uniform("a_point_size");
			glUseProgram(simple_shader_program.get());
			//glDisableVertexAttribArray(color);
			//glVertexAttrib4fv(color, colors);
			glUniform4fv(simple_shader_program.get_uniform("u_color"), 4, colors); 
			glUniformMatrix4fv(simple_shader_program.get_uniform("mvp_matrix"), 
				1, 
				GL_FALSE, 
				(GLfloat*) &mvp.x.x );
		}
	}

	manager::~manager()
	{
		glUseProgram(0);
	}

	program& manager::get()
	{
		ASSERT_LOG(false, "unused: program& manager::get()");
		return simple_shader_program;
	}
}
