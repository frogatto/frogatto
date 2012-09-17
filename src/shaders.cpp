#include <boost/regex.hpp>

#include "asserts.hpp"
#include "foreach.hpp"
#include "formula.hpp"
#include "formula_profiler.hpp"
#include "graphics.hpp"
#include "json_parser.hpp"
#include "level.hpp"
#include "module.hpp"
#include "shaders.hpp"
#include "variant_utils.hpp"

#define WRITE_LOG(_a,_b) if( !(_a) ) { std::ostringstream _s; _s << __FILE__ << ":" << __LINE__ << " ASSERTION FAILED: " << _b << "\n"; std::cerr << _s.str(); return; }

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

namespace {
	std::map<std::string, gles2::program_ptr> shader_programs;
}

program::program() 
	: object_(0), vtx_coord_(-1), col_coord_(-1)
{
	environ_ = this;
	tex_coord_[0] = tex_coord_[1] = -1;
}


program::program(const std::string& name, const shader& vs, const shader& fs)
	: object_(0), vtx_coord_(-1), col_coord_(-1)
{
	environ_ = this;
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
	WRITE_LOG(it != uniforms_.end(), "No uniform found with name: " << key);
	const actives& u = it->second;
	switch(u.type) {
	case GL_FLOAT: {
		glUniform1f(u.location, GLfloat(value.as_decimal().as_float()));
		break;
	}
	case GL_FLOAT_VEC2: {
		WRITE_LOG(value.num_elements() == 2, "Must be four(2) elements in vector.");
		std::vector<GLfloat> f;
		for(size_t n = 0; n < value.num_elements(); ++n) {
			f.push_back(GLfloat(value[n].as_decimal().as_float()));
		}
		glUniform2fv(u.location, u.num_elements, &f[0]);
		break;
	}
	case GL_FLOAT_VEC3: {
		WRITE_LOG(value.num_elements() == 3, "Must be three(3) elements in vector.");
		std::vector<GLfloat> f;
		for(size_t n = 0; n < value.num_elements(); ++n) {
			f.push_back(GLfloat(value[n].as_decimal().as_float()));
		}
		glUniform3fv(u.location, u.num_elements, &f[0]);
		break;
	}
	case GL_FLOAT_VEC4: {
		WRITE_LOG(value.num_elements() == 4, "Must be four(4) elements in vector.");
		std::vector<GLfloat> f;
		for(size_t n = 0; n < value.num_elements(); ++n) {
			f.push_back(GLfloat(value[n].as_decimal().as_float()));
		}
		glUniform4fv(u.location, u.num_elements, &f[0]);
		break;
	}
	case GL_INT:		glUniform1i(u.location, value.as_int()); break;
	case GL_INT_VEC2:	
		WRITE_LOG(value.num_elements() == 2, "Must be two(2) elements in vec.");
		glUniform2i(u.location, value[0].as_int(), value[1].as_int()); 
		break;
	case GL_INT_VEC3:	
		WRITE_LOG(value.num_elements() == 3, "Must be three(3) elements in vec.");
		glUniform3iv(u.location, u.num_elements, &value.as_list_int()[0]); 
		break;
	case GL_INT_VEC4: 	
		WRITE_LOG(value.num_elements() == 4, "Must be four(4) elements in vec.");
		glUniform4iv(u.location, u.num_elements, &value.as_list_int()[0]); 
		break;
	case GL_BOOL:		glUniform1i(u.location, value.as_bool()); break;
	case GL_BOOL_VEC2:	
		WRITE_LOG(value.num_elements() == 2, "Must be two(2) elements in vec.");
		glUniform2i(u.location, value[0].as_bool(), value[1].as_bool()); 
		break;
	case GL_BOOL_VEC3:	
		WRITE_LOG(value.num_elements() == 3, "Must be three(3) elements in vec.");
		glUniform3i(u.location, value[0].as_bool(), value[1].as_bool(), value[2].as_bool()); 
		break;
	case GL_BOOL_VEC4:
		WRITE_LOG(value.num_elements() == 4, "Must be four(4) elements in vec.");
		glUniform4i(u.location, value[0].as_bool(), value[1].as_bool(), value[2].as_bool(), value[3].as_bool()); 
		break;
	case GL_FLOAT_MAT2:	{
		WRITE_LOG(value.num_elements() == 4, "Must be four(4) elements in matrix.");
		std::vector<GLfloat> f;
		for(size_t n = 0; n < value.num_elements(); ++n) {
			f.push_back(GLfloat(value[n].as_decimal().as_float()));
		}
		glUniformMatrix2fv(u.location, u.num_elements, GL_FALSE, &f[0]);
		break;
	}
	case GL_FLOAT_MAT3: {
		WRITE_LOG(value.num_elements() == 9, "Must be nine(9) elements in matrix.");
		std::vector<GLfloat> f;
		for(size_t n = 0; n < value.num_elements(); ++n) {
			f.push_back(GLfloat(value[n].as_decimal().as_float()));
		}
		glUniformMatrix3fv(u.location, u.num_elements, GL_FALSE, &f[0]);
		break;
	}
	case GL_FLOAT_MAT4: {
		WRITE_LOG(value.num_elements() == 16, "Must be four(16) elements in matrix.");
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
		WRITE_LOG(false, "Unhandled uniform type: " << it->second.type);
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
#if defined(USE_GLES2)
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
#endif
	return variant();
}

void program::set_value(const std::string& key, const variant& value)
{
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
#if defined(USE_GLES2)
			for(size_t n = 0; n < 16; n++) {
				v.push_back(variant(((GLfloat*)(&gles2::get_mvp_matrix().x.x))[n]));
			}
#endif
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

variant program::write()
{
	variant_builder res;
	res.add("program", name());
	res.add("vertex", vs_.name());
	res.add("fragment", fs_.name());
	res.add("attributes", stored_attributes_);
	return res.build();
}

void program::set_attributes(const variant& node)
{
	std::cerr << "shader program: " << name();
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
	stored_attributes_ = node;
}


void program::load_shaders(const std::string& shader_data)
{
	variant node = json::parse(shader_data);
	//std::cerr << "load_shaders: " << node << std::endl;
	ASSERT_LOG(node.is_map() && node.has_key("shaders") && node.has_key("programs"),
		"shaders.cfg must be a map with \"shaders\" and \"programs\" attributes.");
	for(size_t n = 0; n < node["programs"].num_elements(); ++n) {
		const variant& prog = node["programs"][n];
		ASSERT_LOG(prog.has_key("vertex") 
			&& prog.has_key("fragment") 
			&& prog.has_key("name"),
			"Program's must contain \"vertex\", \"fragment\" and \"name\" attributes.");
		const std::string vs_name = prog["vertex"].as_string();
		const std::string fs_name = prog["fragment"].as_string();

		ASSERT_LOG(node["shaders"].has_key("vertex") 
			&& node["shaders"]["vertex"].has_key(vs_name),
			"No key \"" << vs_name << "\" found under \"vertex\" attribute.");
		ASSERT_LOG(node["shaders"].has_key("fragment") 
			&& node["shaders"]["fragment"].has_key(fs_name),
			"No key \"" << vs_name << "\" found under \"fragment\" attribute.");
		std::string vert_data = node["shaders"]["vertex"][vs_name].as_string();
		std::string frag_data = node["shaders"]["fragment"][fs_name].as_string();

		// Simple test to differntiate shaders as strings, compared to shaders in files.
		// i.e. shaders as strings will have "void main" stanzas, it would be kind of
		// pathological to create a file containing "void main" as part of the filename.
		const boost::regex re("void\\s+main");
		if(boost::regex_search(vert_data, re) == false) {
			// Try loading as file.
			vert_data = sys::read_file(module::map_file("data/" + vert_data));
		}
		if(boost::regex_search(frag_data, re) == false) {
			// Try loading as file.
			frag_data = sys::read_file(module::map_file("data/" + frag_data));
		}

		gles2::shader v_shader(GL_VERTEX_SHADER, vs_name, vert_data);
		gles2::shader f_shader(GL_FRAGMENT_SHADER, fs_name, frag_data);
		const std::string& program_name = prog["name"].as_string();
		add_shader(program_name, v_shader, f_shader, prog["attributes"]);

		std::cerr << "Loaded shader program: \"" << program_name << "\" from file. (" 
			<< vs_name << ", " << fs_name << ")." << std::endl;
	}
}

void program::add_shader(const std::string& program_name, 
		const shader& v_shader, 
		const shader& f_shader, 
		const variant& prog)
{
	std::map<std::string, gles2::program_ptr>::iterator it = shader_programs.find(program_name);
	if(it == shader_programs.end()) {
		shader_programs[program_name] = program_ptr(new program(program_name, v_shader, f_shader));
	} else {
		it->second->init(program_name, v_shader, f_shader);
	}
	shader_programs[program_name]->set_attributes(prog);
}

program_ptr program::find_program(const std::string& prog_name)
{
	std::map<std::string, gles2::program_ptr>::const_iterator it = shader_programs.find(prog_name);
	ASSERT_LOG(it != shader_programs.end(), "Shader program \"" << prog_name << "\" not found.");
	return it->second;
}


///////////////////////////////////////////////////////////////////////////
// shader_program

shader_program::shader_program()
{
}

shader_program::shader_program(const variant& node)
{
	init(node);
}

void shader_program::init(const variant& node)
{
	ASSERT_LOG(node.is_map(), "shader attribute must be a map.");
	name_ = node["program"].as_string();
	program_object_ = program::find_program(name_);
	game_logic::formula_callable* e = program_object_->get_environment();
	ASSERT_LOG(e != NULL, "Environment was not set.");

	if(node.has_key("uniforms")) {
		const variant& u = node["uniforms"];
		if(u.has_key("on_create")) {
			create_ = e->create_formula(u["on_create"]);
		}
		if(u.has_key("on_draw")) {
			draw_ = e->create_formula(u["on_draw"]);
		}
	}

	GLint current_program;
	glGetIntegerv(GL_CURRENT_PROGRAM, &current_program);
	// Set default values for uniforms
	glUseProgram(program_object_->get());
	if(create_ != NULL) {
		e->execute_command(create_->execute(*e));
	}
	glUseProgram(current_program);
}

variant shader_program::write()
{
	variant_builder u;
	u.add("program", name());
	u.add("on_create", create_->str());
	u.add("on_draw", draw_->str());
	return u.build();
}

void shader_program::prepare_draw()
{
	//LARGE_INTEGER frequency;
	//LARGE_INTEGER t1, t2;
	//double elapsedTime;
	//QueryPerformanceFrequency(&frequency);
	//QueryPerformanceCounter(&t1);

	glUseProgram(program_object_->get());
	if(draw_ != NULL) {
		game_logic::formula_callable* e = program_object_->get_environment();
		e->execute_command(draw_->execute(*e));
	}

	//QueryPerformanceCounter(&t2);
	//elapsedTime = (t2.QuadPart - t1.QuadPart) * 1000.0 / frequency.QuadPart;
	//std::cerr << elapsedTime << " ms\n";
}

variant shader_program::get_value(const std::string& key) const
{
	return program_object_->get_value(key);
}

void shader_program::set_value(const std::string& key, const variant& value)
{
	program_object_->set_value(key, value);
}

program_ptr shader_program::shader() const 
{ 
	ASSERT_LOG(program_object_ != NULL, "null shader program");
	return program_object_;
}

void shader_program::clear()
{
	program_object_.reset();
	create_.reset();
	draw_.reset();
	name_.clear();
}

}
