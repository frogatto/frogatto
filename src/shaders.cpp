#include "asserts.hpp"
#include "foreach.hpp"
#include "formula.hpp"
#include "formula_profiler.hpp"
#include "graphics.hpp"
#include "shaders.hpp"

namespace gles2 {

shader::shader(GLenum type, const std::string& name, const std::string& code)
	: type_(type), shader_(0), name_(name)
{
	ASSERT_LOG(compile(code), "Error compiling shader for " << name_);
}

bool shader::compile(const std::string& code)
{
	GLint compiled;
	if(shader_) {
		glDeleteShader(shader_);
		shader_ = 0;
	}

	shader_ = glCreateShader(type_);
	if(shader_ == 0) {
		std::cerr << "Enable to create shader." << std::endl;
		return false;
	}
	const GLchar* shader_code = reinterpret_cast<const GLchar*>(code.c_str());
	glShaderSource(shader_, 1, &shader_code, NULL);
	glCompileShader(shader_);
	glGetShaderiv(shader_, GL_COMPILE_STATUS, &compiled);
	if(!compiled) {
		GLint info_len = 0;
		glGetShaderiv(shader_, GL_INFO_LOG_LENGTH, &info_len);
		if(info_len > 1) {
			std::vector<char> info_log;
			info_log.resize(info_len);
			glGetShaderInfoLog(shader_, info_log.capacity(), NULL, &info_log[0]);
			std::string s(info_log.begin(), info_log.end());
			std::cerr << "Error compiling shader: " << s << std::endl;
		}
		glDeleteShader(shader_);
		shader_ = 0;
		return false;
	}
	return true;
}

program::program() 
	: object_(0), vtx_coord_(0), col_coord_(0), environ_(NULL)
{
	tex_coord_[0] = tex_coord_[1] = 0;
}


program::program(const std::string& name, const shader& vs, const shader& fs)
	: object_(0), vtx_coord_(-1), col_coord_(-1), environ_(NULL)
{
	tex_coord_[0] = tex_coord_[1] = -1;
	init(name, vs, fs);
}

void program::init(const std::string& name, const shader& vs, const shader& fs)
{
	name_ = name;
	vs_ = vs;
	fs_ = fs;
	ASSERT_LOG(link(), "Error linking program: " << name_);
}

bool program::link()
{
	if(object_) {
		glDeleteProgram(object_);
		object_ = 0;
	}
	object_ = glCreateProgram();
	ASSERT_LOG(object_ != 0, "Unable to create program object.");
	glAttachShader(object_, vs_.get());
	glAttachShader(object_, fs_.get());
	glLinkProgram(object_);
	GLint linked = 0;
	glGetProgramiv(object_, GL_LINK_STATUS, &linked);
	if(!linked) {
		GLint info_len = 0;
		glGetProgramiv(object_, GL_INFO_LOG_LENGTH, &info_len);
		if(info_len > 1) {
			std::vector<char> info_log;
			info_log.resize(info_len);
			glGetProgramInfoLog(object_, info_log.capacity(), NULL, &info_log[0]);
			std::string s(info_log.begin(), info_log.end());
			std::cerr << "Error linking object: " << s << std::endl;
		}
		glDeleteProgram(object_);
		object_ = 0;
		return false;
	}
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	return queryUniforms() && queryAttributes();
}

GLuint program::get_attribute(const std::string& attr) const
{
	std::map<std::string, actives>::const_iterator it = attribs_.find(attr);
	ASSERT_LOG(it != attribs_.end(), "Attribute \"" << attr << "\" not found in list.");
	return it->second.location;
}

GLuint program::get_uniform(const std::string& attr) const
{
	std::map<std::string, actives>::const_iterator it = uniforms_.find(attr);
	ASSERT_LOG(it != uniforms_.end(), "Uniform \"" << attr << "\" not found in list.");
	return it->second.location;
}

bool program::queryAttributes()
{
	GLint active_attribs;
	glGetProgramiv(object_, GL_ACTIVE_ATTRIBUTES, &active_attribs);
	GLint attributes_max_len;
	glGetProgramiv(object_, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &attributes_max_len);
	std::vector<char> name;
	name.resize(attributes_max_len+1);
	for(int i = 0; i < active_attribs; i++) {
		actives a;
		GLsizei size;
		glGetActiveAttrib(object_, i, name.size(), &size, &a.num_elements, &a.type, &name[0]);
		a.name = std::string(&name[0], &name[size]);
		a.location = glGetAttribLocation(object_, a.name.c_str());
		ASSERT_LOG(a.location >= 0, "Unable to determine the location of the attribute: " << a.name);
		attribs_[a.name] = a;
	}
	return true;
}

bool program::queryUniforms()
{
	GLint active_uniforms;
	glGetProgramiv(object_, GL_ACTIVE_UNIFORMS, &active_uniforms);
	GLint uniform_max_len;
	glGetProgramiv(object_, GL_ACTIVE_UNIFORM_MAX_LENGTH, &uniform_max_len);
	std::vector<char> name;
	name.resize(uniform_max_len+1);
	for(int i = 0; i < active_uniforms; i++) {
		actives u;
		GLsizei size;
		glGetActiveUniform(object_, i, name.size(), &size, &u.num_elements, &u.type, &name[0]);
		u.name = std::string(&name[0], &name[size]);
		u.location = glGetUniformLocation(object_, u.name.c_str());
		ASSERT_LOG(u.location >= 0, "Unable to determine the location of the uniform: " << u.name);
		uniforms_[u.name] = u;
	}
	return true;
}

void program::set_uniform(const std::string& key, const variant& value)
{
	std::map<std::string, actives>::iterator it = uniforms_.find(key);
	ASSERT_LOG(it != uniforms_.end(), "No uniform found with name: " << key);
	//ASSERT_LOG(it->second.num_elements > 1 && value.num_elements() != it->second.num_elements,
	//	"Number of elements doesn't match: " << it->second.num_elements << " != " << value.num_elements());
	const actives& u = it->second;
	switch(u.type) {
	case GL_FLOAT: {
		glUniform1f(u.location, GLfloat(value.as_decimal().as_float()));
		break;
	}
	case GL_FLOAT_VEC2: {
		std::vector<GLfloat> f;
		for(size_t n = 0; n < value.num_elements(); ++n) {
			f.push_back(GLfloat(value[n].as_decimal().as_float()));
		}
		glUniform2fv(u.location, u.num_elements, &f[0]);
		break;
	}
	case GL_FLOAT_VEC3: {
		std::vector<GLfloat> f;
		for(size_t n = 0; n < value.num_elements(); ++n) {
			f.push_back(GLfloat(value[n].as_decimal().as_float()));
		}
		glUniform3fv(u.location, u.num_elements, &f[0]);
		break;
	}
	case GL_FLOAT_VEC4: {
		std::vector<GLfloat> f;
		for(size_t n = 0; n < value.num_elements(); ++n) {
			f.push_back(GLfloat(value[n].as_decimal().as_float()));
		}
		glUniform4fv(u.location, u.num_elements, &f[0]);
		break;
	}
	case GL_INT:		glUniform1i(u.location, value.as_int()); break;
	case GL_INT_VEC2:	glUniform2i(u.location, value[0].as_int(), value[1].as_int()); break;
	case GL_INT_VEC3:	glUniform3iv(u.location, u.num_elements, &value.as_list_int()[0]); break;
	case GL_INT_VEC4: 	glUniform4iv(u.location, u.num_elements, &value.as_list_int()[0]); break;
	case GL_BOOL:		glUniform1i(u.location, value.as_bool()); break;
	case GL_BOOL_VEC2:	glUniform2i(u.location, value[0].as_bool(), value[1].as_bool()); break;
	case GL_BOOL_VEC3:	
		glUniform3i(u.location, value[0].as_bool(), value[1].as_bool(), value[2].as_bool()); 
		break;
	case GL_BOOL_VEC4:
		glUniform4i(u.location, value[0].as_bool(), value[1].as_bool(), value[2].as_bool(), value[3].as_bool()); 
		break;
	case GL_FLOAT_MAT2:	{
		std::vector<GLfloat> f;
		for(size_t n = 0; n < value.num_elements(); ++n) {
			f.push_back(GLfloat(value[n].as_decimal().as_float()));
		}
		glUniformMatrix2fv(u.location, u.num_elements, GL_FALSE, &f[0]);
		break;
	}
	case GL_FLOAT_MAT3: {
		std::vector<GLfloat> f;
		for(size_t n = 0; n < value.num_elements(); ++n) {
			f.push_back(GLfloat(value[n].as_decimal().as_float()));
		}
		glUniformMatrix3fv(u.location, u.num_elements, GL_FALSE, &f[0]);
		break;
	}
	case GL_FLOAT_MAT4: {
		std::vector<GLfloat> f;
		for(size_t n = 0; n < value.num_elements(); ++n) {
			f.push_back(GLfloat(value[n].as_decimal().as_float()));
		}
		glUniformMatrix4fv(u.location, u.num_elements, GL_FALSE, &f[0]);
		break;
	}

	case GL_SAMPLER_2D:		glUniform1i(u.location, value.as_int()); break;

	case GL_SAMPLER_CUBE:
	default:
		ASSERT_LOG(false, "Unhandled uniform type: " << it->second.type);
	}
}

namespace {
	class uniforms_callable : public game_logic::formula_callable 
	{
		program_ptr program_;
		variant get_value(const std::string& key) const 
		{
			return variant();
		}
		void set_value(const std::string& key, const variant& value) 
		{
			program_->set_uniform(key, value);
		}
	public:
		explicit uniforms_callable(const program& p) 
			: program_(const_cast<program*>(&p))
		{}
	};
}

variant program::get_value(const std::string& key) const
{
	if(key == "uniforms") {
		return variant(new uniforms_callable(*this));
	} else if(key == "alpha") {
		return variant(get_alpha());
	} else if(key == "color") {
		std::vector<variant> v;
		for(int n = 0; n < 4; ++n) {
			v.push_back(variant(get_color()[n]));
		}
		return variant(&v);
	} else if(key == "point_size") {
		//return variant(get_point_size());
		GLfloat pt_size;
		glGetFloatv(GL_POINT_SIZE, &pt_size);
		return variant(pt_size);
	} else if(key == "mvp_matrix" ) {
		std::vector<variant> v;
		for(size_t n = 0; n < 16; n++) {
			v.push_back(variant(((GLfloat*)(&gles2::get_mvp_matrix().x.x))[n]));
		}
		return variant(&v);
	}
	return variant();
}

void program::set_value(const std::string& key, const variant& value)
{
}

void program::configure(const variant& node, game_logic::formula_callable* e)
{
	ASSERT_LOG(e != NULL, "callable environment can't be NULL.");
	ASSERT_LOG(node["program"].as_string() == name(), 
		"name specified in shader definition is not the same as the program name");
	environ_ = e;
	if(node.has_key("uniforms")) {
		foreach(const variant_pair& p, node["uniforms"].as_map()) {
			if(p.first.as_string() == "on_create") {
				create_ = e->create_formula(p.second);
			}
			if(p.first.as_string() == "on_draw") {
				draw_ = e->create_formula(p.second);
			}
		}
	}

	if(node.has_key("attributes")) {
		foreach(const variant_pair&p, node["attributes"].as_map()) {
			const std::string key = p.first.as_string();
			if(key == "vertex") {
				vtx_coord_ = get_attribute(p.second.as_string());
			} else if(key == "color") {
				col_coord_ = get_attribute(p.second.as_string());
			} else if(key == "texcoord" || key == "texcoord0") {
				tex_coord_[0] = get_attribute(p.second.as_string());
			} else if(key == "texcoord1") {
				tex_coord_[1] = get_attribute(p.second.as_string());
			}
		}
	}

	GLint current_program;
	glGetIntegerv(GL_CURRENT_PROGRAM, &current_program);
	// Set default values for uniforms
	glUseProgram(get());
	if(create_ != NULL) {
		environ_->execute_command(create_->execute(*environ_));
	}
	glUseProgram(current_program);
}

namespace {
	class get_mvp_matrix_function : public game_logic::function_expression {
	public:
		explicit get_mvp_matrix_function(const args_list& args)
		 : function_expression("get_mvp_matrix", args, 0, 0)
		{}
	private:
		variant execute(const game_logic::formula_callable& variables) const {
			game_logic::formula::fail_if_static_context();
			std::vector<variant> v;
			for(size_t n = 0; n < 16; n++) {
				v.push_back(variant(((GLfloat*)(&gles2::get_mvp_matrix().x.x))[n]));
			}
			return variant(&v);
		}
	};

	class shader_symbol_table : public game_logic::function_symbol_table
	{
	public:
		shader_symbol_table()
		{}

		game_logic::expression_ptr create_function(
			const std::string& fn,
			const std::vector<game_logic::expression_ptr>& args,
			const game_logic::formula_callable_definition* callable_def) const
		{
			if(fn == "get_mvp_matrix") {
				return game_logic::expression_ptr(new get_mvp_matrix_function(args));
			} 
			return function_symbol_table::create_function(fn, args, callable_def);
		}
	};

	game_logic::function_symbol_table& get_shader_symbol_table()
	{
		static shader_symbol_table table;
		return table;
	}
}

game_logic::formula_ptr program::create_formula(const variant& v)
{
	return game_logic::formula_ptr(new game_logic::formula(v, &get_shader_symbol_table()));
}

bool program::execute_command(const variant& var)
{
	bool result = true;
	if(var.is_null()) {
		return result;
	}

	if(var.is_list()) {
		const int num_elements = var.num_elements();
		for(int n = 0; n != num_elements; ++n) {
			if(var[n].is_null() == false) {
				result = execute_command(var[n]) && result;
			}
		}
	} else {
		game_logic::command_callable* cmd = var.try_convert<game_logic::command_callable>();
		if(cmd != NULL) {
			cmd->execute(*this);
		}
	}
	return result;
}

void program::prepare_draw()
{
	//LARGE_INTEGER frequency;
	//LARGE_INTEGER t1, t2;
	//double elapsedTime;
	//QueryPerformanceFrequency(&frequency);
	//QueryPerformanceCounter(&t1);

	glUseProgram(get());
	if(draw_ != NULL) {
		environ_->execute_command(draw_->execute(*environ_));
	}

	//QueryPerformanceCounter(&t2);
	//elapsedTime = (t2.QuadPart - t1.QuadPart) * 1000.0 / frequency.QuadPart;
	//std::cerr << elapsedTime << " ms\n";
}

void program::disable_vertex_attrib(GLint)
{
	::glDisableVertexAttribArray(vtx_coord_);
	::glDisableVertexAttribArray(tex_coord_[0]);
	::glDisableVertexAttribArray(tex_coord_[1]);
	::glDisableVertexAttribArray(col_coord_);
}

void program::vertex_attrib_array(GLint ndx, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* ptr)
{
	::glVertexAttribPointer(ndx, size, type, normalized, stride, ptr);
	::glEnableVertexAttribArray(ndx);
}

void program::vertex_array(GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* ptr)
{
	vertex_attrib_array(vtx_coord_, size, type, normalized, stride, ptr);
}

void program::texture_array(GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* ptr)
{
	vertex_attrib_array(tex_coord_[0], size, type, normalized, stride, ptr);
}

void program::color_array(GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* ptr)
{
	vertex_attrib_array(col_coord_, size, type, normalized, stride, ptr);
}

}
