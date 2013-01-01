#include "asserts.hpp"
#include "filesystem.hpp"
#include "graphics.hpp"
#include "gles2.hpp"
#include "json_parser.hpp"
#include "level.hpp"
#include "module.hpp"

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

extern "C" {

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
		"uniform vec4 u_color;\n"
		"void main()\n"
		"{\n"
		"	gl_FragColor = u_color;\n"
		"}\n";
	const std::string vs1 = 
		"uniform mat4 mvp_matrix;\n"
		"uniform float u_point_size;\n"
		"attribute vec4 a_position;\n"
		"void main()\n"
		"{\n"
		"	gl_PointSize = u_point_size;\n"
		"	gl_Position = mvp_matrix * a_position;\n"
		"}\n";
    const std::string simple_shader_info = 
		"{\"shader\": {\n"
		"    \"program\": \"simple_shader\",\n"
		"    \"create\": \"\",\n"
		"    \"draw\": \"[set(uniforms.mvp_matrix, mvp_matrix), set(uniforms.u_color,color), set(uniforms.u_point_size, point_size)]\",\n"
		"}}\n";
	const std::string simple_attribute_info = 
		"{\n"
		"    \"attributes\": {\n"
		"        \"vertex\": \"a_position\",\n"
		"    },\n"
		"}\n";

	const std::string fs_col = 
		"varying vec4 v_color;\n"
		"void main()\n"
		"{\n"
		"	gl_FragColor = v_color;\n"
		"}\n";
	const std::string vs_col = 
		"uniform mat4 mvp_matrix;\n"
		"uniform float u_point_size;\n"
		"attribute vec4 a_position;\n"
		"attribute vec4 a_color;\n"
		"varying vec4 v_color;\n"
		"void main()\n"
		"{\n"
		"	v_color = a_color;\n"
		"	gl_PointSize = u_point_size;\n"
		"	gl_Position = mvp_matrix * a_position;\n"
		"}\n";
    const std::string simple_col_shader_info = 
		"{\"shader\": {\n"
		"    \"program\": \"simple_col_shader\",\n"
		"    \"create\": \"\",\n"
		"    \"draw\": \"[set(uniforms.mvp_matrix, mvp_matrix),set(uniforms.u_point_size,point_size)]\",\n"
		"}}\n";
	const std::string simple_col_attribute_info = 
		"{\n"
		"    \"attributes\": {\n"
		"        \"vertex\": \"a_position\",\n"
		"        \"color\": \"a_color\",\n"
		"    },\n"
		"}\n";

	const std::string fs_tex = 
		"uniform sampler2D u_tex_map;\n"
		"uniform vec4 u_color;\n"
		"varying vec2 v_texcoord;\n"
		"void main()\n"
		"{\n"
		"	gl_FragColor = texture2D(u_tex_map, v_texcoord) * u_color;\n"
		"}\n";
	const std::string vs_tex = 
		"uniform mat4 mvp_matrix;\n"
		"attribute vec4 a_position;\n"
		"attribute vec2 a_texcoord;\n"
		"varying vec2 v_texcoord;\n"
		"void main()\n"
		"{\n"
		"	v_texcoord = a_texcoord;\n"
		"	gl_Position = mvp_matrix * a_position;\n"
		"}\n";
	const std::string tex_shader_info = 
		"{\"shader\": {\n"
        "    \"program\": \"tex_shader\",\n"
		"    \"create\": \"[set(uniforms.u_tex_map, 0)]\",\n"
		"    \"draw\": \"[set(uniforms.mvp_matrix,mvp_matrix),set(uniforms.u_color,color)]\",\n"
		"}}\n";
	const std::string tex_attribute_info = 
		"{\n"
        "    \"attributes\": {\n"
        "        \"vertex\": \"a_position\",\n"
        "        \"texcoord\": \"a_texcoord\",\n"
        "    },\n"
		"}\n";

	const std::string fs_texcol = 
		"uniform sampler2D u_tex_map;\n"
		"varying vec4 v_color;\n"
		"varying vec2 v_texcoord;\n"
		"void main()\n"
		"{\n"
		"	gl_FragColor = texture2D(u_tex_map, v_texcoord) * v_color;\n"
		"}\n";
	const std::string vs_texcol = 
		"uniform mat4 mvp_matrix;\n"
		"attribute vec4 a_position;\n"
		"attribute vec4 a_color;\n"
		"attribute vec2 a_texcoord;\n"
		"varying vec2 v_texcoord;\n"
		"varying vec4 v_color;\n"
		"void main()\n"
		"{\n"
		"	v_color = a_color;\n"
		"	v_texcoord = a_texcoord;\n"
		"	gl_Position = mvp_matrix * a_position;\n"
		"}\n";
	const std::string texcol_shader_info = 
		"{\"shader\": {\n"
        "    \"program\": \"texcol_shader\",\n"
		"    \"create\": \"[set(uniforms.u_tex_map, 0)]\",\n"
		"    \"draw\": \"[set(uniforms.mvp_matrix,mvp_matrix),set(attributes.a_color,color)]\",\n"
		"}}\n";
	const std::string texcol_attribute_info = 
		"{\n"
        "    \"attributes\": {\n"
        "        \"vertex\": \"a_position\",\n"
        "        \"texcoord\": \"a_texcoord\",\n"
		"        \"color\": \"a_color\",\n"
        "    },\n"
		"}\n";

	static gles2::shader_ptr tex_shader_program;
	static gles2::shader_ptr texcol_shader_program;
	static gles2::shader_ptr simple_shader_program;
	static gles2::shader_ptr simple_col_shader_program;

	std::stack<gles2::shader_ptr> shader_stack;
	gles2::shader_ptr active_shader_program;

	struct blend_mode 
	{
		GLenum blend_src_mode;
		GLenum blend_dst_mode;
		bool blend_enabled;
		blend_mode(GLenum src, GLenum dst, bool en) 
			: blend_src_mode(src), blend_dst_mode(dst), blend_enabled(en)
		{}
	};
	std::stack<blend_mode> blend_stack;
}

namespace gles2 {
	fixed_program::fixed_program() : program(), vtx_coord_(-1), col_coord_(-1)
	{
		tex_coord_[0] = tex_coord_[1] = -1;
	}

	fixed_program::fixed_program(const std::string& name, const shader& vs, const shader& fs) 
		: program(name, vs, fs), vtx_coord_(-1), col_coord_(-1)
	{
		tex_coord_[0] = tex_coord_[1] = -1;
	}

	void fixed_program::vertex_array(GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* ptr)
	{
		vertex_attrib_array(vtx_coord_, size, type, normalized, stride, ptr);
	}

	void fixed_program::texture_array(GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* ptr)
	{
		vertex_attrib_array(tex_coord_[0], size, type, normalized, stride, ptr);
	}

	void fixed_program::color_array(GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* ptr)
	{
		vertex_attrib_array(col_coord_, size, type, normalized, stride, ptr);
	}

	void fixed_program::add_shader(const std::string& program_name, 
			const shader& v_shader, 
			const shader& f_shader,
			const variant& prog)
	{
		std::map<std::string, gles2::program_ptr>::iterator it = program::get_shaders().find(program_name);
		if(it == program::get_shaders().end()) {
			program::get_shaders()[program_name] = fixed_program_ptr(new fixed_program(program_name, v_shader, f_shader));
		} else {
			it->second->init(program_name, v_shader, f_shader);
		}
		program::get_shaders()[program_name]->set_fixed_attributes(prog);
	}

	void fixed_program::set_fixed_attributes(const variant& node)
	{
		program::set_fixed_attributes(node);
		std::cerr << "shader program: " << name() << "(" << get() << ")";
		if(node.has_key("vertex")) {
			vtx_coord_ = get_attribute(node["vertex"].as_string());
			std::cerr << ", vtx_coord: " << vtx_coord_;
		} 
		if(node.has_key("color")) {
			col_coord_ = get_attribute(node["color"].as_string());
			std::cerr << ", col_coord: " << col_coord_;
		} 
		if(node.has_key("colour")) {
			col_coord_ = get_attribute(node["colour"].as_string());
			std::cerr << ", col_coord: " << col_coord_;
		} 
		if(node.has_key("texcoord")) {
			tex_coord_[0] = get_attribute(node["texcoord"].as_string());
			std::cerr << ", tex_coord0: " << tex_coord_[0];
		} 
		if(node.has_key("texcoord0")) {
			tex_coord_[0] = get_attribute(node["texcoord0"].as_string());
			std::cerr << ", tex_coord0: " << tex_coord_[0];
		} 
		if(node.has_key("texcoord1")) {
			tex_coord_[1] = get_attribute(node["texcoord1"].as_string());
			std::cerr << ", tex_coord1: " << tex_coord_[1];
		}
		std::cerr << std::endl;
	}

	shader_ptr get_tex_shader()
	{
		return tex_shader_program;
	}

	shader_ptr get_texcol_shader()
	{
		return texcol_shader_program;
	}

	shader_ptr get_simple_shader()
	{
		return simple_shader_program;
	}

	shader_ptr get_simple_col_shader()
	{
		return simple_col_shader_program;
	}

	shader_ptr active_shader()
	{
		return active_shader_program;
	}

	const mat4& get_mvp_matrix()
	{
		static mat4 mvp = mat4::identity();
	#if !defined(GL_ES_VERSION_2_0)
		glGetFloatv(GL_MODELVIEW_MATRIX, &modelview_matrix.x.x);
		glGetFloatv(GL_PROJECTION_MATRIX, &proj_matrix.x.x);
	#endif
		mvp = proj_matrix * modelview_matrix;
		return mvp;
	}

	GLfloat get_alpha()
	{
		return colors[3];
	}

	GLfloat* get_color()
	{
		return colors;
	}

	GLfloat get_point_size()
	{
		GLfloat pt_size;
		glGetFloatv(GL_POINT_SIZE, &pt_size);
		return pt_size;
	}

	void init_default_shader()
	{
		gles2::shader v1(GL_VERTEX_SHADER, "simple_vertex_shader", vs1);
		gles2::shader f1(GL_FRAGMENT_SHADER, "simple_fragment_shader", fs1);
		fixed_program::add_shader("simple_shader", v1, f1, json::parse(simple_attribute_info)["attributes"]);
		simple_shader_program.reset(new shader_program());
		simple_shader_program->configure(json::parse(simple_shader_info)["shader"]);
		simple_shader_program->init(0);

		gles2::shader v1_col(GL_VERTEX_SHADER, "simple_col_vertex_shader", vs_col);
		gles2::shader f1_col(GL_FRAGMENT_SHADER, "simple_col_fragment_shader", fs_col);
		fixed_program::add_shader("simple_col_shader", v1_col, f1_col, json::parse(simple_col_attribute_info)["attributes"]);
		simple_col_shader_program.reset(new shader_program());
		simple_col_shader_program->configure(json::parse(simple_col_shader_info)["shader"]);
		simple_col_shader_program->init(0);

		gles2::shader v_tex(GL_VERTEX_SHADER, "tex_vertex_shader", vs_tex);
		gles2::shader f_tex(GL_FRAGMENT_SHADER, "tex_fragment_shader", fs_tex);
		fixed_program::add_shader("tex_shader", v_tex, f_tex, json::parse(tex_attribute_info)["attributes"]);
		tex_shader_program.reset(new shader_program());
		tex_shader_program->configure(json::parse(tex_shader_info)["shader"]);
		tex_shader_program->init(0);

		gles2::shader v_texcol(GL_VERTEX_SHADER, "texcol_vertex_shader", vs_texcol);
		gles2::shader f_texcol(GL_FRAGMENT_SHADER, "texcol_fragment_shader", fs_texcol);
		fixed_program::add_shader("texcol_shader", v_texcol, f_texcol, json::parse(texcol_attribute_info)["attributes"]);
		texcol_shader_program.reset(new shader_program());
		texcol_shader_program->configure(json::parse(texcol_shader_info)["shader"]);
		texcol_shader_program->init(0);

		matrix_mode = GL_PROJECTION;
		p_mat_stack.empty();
		mv_mat_stack.empty();
		proj_matrix = mat4::identity();
		modelview_matrix = mat4::identity();
		colors[0] = 1.0f;
		colors[1] = 1.0f;
		colors[2] = 1.0f;
		colors[3] = 1.0f;
		glActiveTexture(GL_TEXTURE0);

		std::string shader_file = module::map_file("data/shaders.cfg");
		if(sys::file_exists(shader_file)) {
			program::load_shaders(sys::read_file(shader_file));
		}
		active_shader_program = tex_shader_program;
	}

	manager::manager(shader_ptr shader)
	{
		// Reset errors, so we can track errors that happened here.
		glGetError();

		GLint blend_src_mode;
		GLint blend_dst_mode;
		// Save current blend mode
		glGetIntegerv(GL_BLEND_SRC, &blend_src_mode);
		glGetIntegerv(GL_BLEND_DST, &blend_dst_mode);
		blend_stack.push(blend_mode(blend_src_mode, blend_dst_mode, glIsEnabled(GL_BLEND) != 0));

		if(shader == NULL || active_shader_program == shader) {
			return;
		}

		if(active_shader_program != shader) {
			shader_stack.push(active_shader_program);
			active_shader_program = shader;
		}
		ASSERT_LOG(active_shader_program != NULL, "Active shader was NULL");
		active_shader_program->prepare_draw();

		GLenum err = glGetError();
		ASSERT_LOG(err == GL_NONE, "Error in shader code: " << shader->name() << " : 0x" << std::hex << err);
	}

	manager::~manager()
	{
		blend_mode bm(blend_stack.top());
		blend_stack.pop();
		if(bm.blend_enabled) {
			glEnable(GL_BLEND);
		} else {
			glDisable(GL_BLEND);
		}
		glBlendFunc(bm.blend_src_mode, bm.blend_dst_mode);

		active_shader_program->shader()->disable_vertex_attrib(-1);
		if(shader_stack.empty() == false) {
			active_shader_program = shader_stack.top();
			shader_stack.pop();
		} else {
			active_shader_program = tex_shader_program;
		}
		glUseProgram(active_shader_program->shader()->get());
	}
}
