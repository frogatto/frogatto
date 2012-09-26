#include <boost/regex.hpp>

#include "array_callable.hpp"
#include "asserts.hpp"
#include "custom_object.hpp"
#include "foreach.hpp"
#include "formula.hpp"
#include "formula_profiler.hpp"
#include "graphics.hpp"
#include "json_parser.hpp"
#include "level.hpp"
#include "module.hpp"
#include "shaders.hpp"
#include "variant_utils.hpp"
#if defined(WIN32)
#include "win_profile_timer.hpp"
#endif

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
	: object_(0)
{
	environ_ = this;
}


program::program(const std::string& name, const shader& vs, const shader& fs)
	: object_(0)
{
	environ_ = this;
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
	//ASSERT_LOG(it != uniforms_.end(), "Uniform \"" << attr << "\" not found in list.");
	if(it == uniforms_.end()) {
		return 0xffffffffUL;
	}
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

variant program::get_uniform_value(const std::string& key) const
{
	std::map<std::string, actives>::const_iterator it = uniforms_.find(key);
	ASSERT_LOG(it != uniforms_.end(), "No uniform found with name: " << key);
	return it->second.last_value;
}

void program::set_uniform(const std::string& key, const variant& value)
{
	std::map<std::string, actives>::iterator it = uniforms_.find(key);
	WRITE_LOG(it != uniforms_.end(), "No uniform found with name: " << key);
	const actives& u = it->second;
	it->second.last_value = value;
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
			//GLfloat f[16];
			//glGetUniformfv(program_->get(), program_->get_uniform(key), f);
			//GLenum err = glGetError();
			//ASSERT_LOG(err == GL_NONE, "glGetUniformfv OpenGL error: 0x" << std::hex << err);
			// XXX fixme to check type of uniform for number of elements to return.
			//return variant(f[0]);
			return program_->get_uniform_value(key);
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

	class attributes_callable : public game_logic::formula_callable 
	{
		program_ptr program_;
		variant get_value(const std::string& key) const 
		{
			return program_->get_attributes_value(key);
		}
		void set_value(const std::string& key, const variant& value) 
		{
			program_->set_attributes(key, value);
		}
	public:
		explicit attributes_callable(const program& p) 
			: program_(const_cast<program*>(&p))
		{}
	};
}

variant program::get_value(const std::string& key) const
{
#if defined(USE_GLES2)
	if(key == "uniforms") {
		return variant(new uniforms_callable(*this));
	} else if(key == "attributes") {
		return variant(new attributes_callable(*this));
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
	GLenum convert_mode(const std::string& smode)
	{
		if(smode == "points") {
			return GL_POINTS;
		} else if(smode == "lines") {
			return GL_LINES;
		} else if(smode == "line_strips") {
			return GL_LINE_STRIP;
		} else if(smode == "line_loop") {
			return GL_LINE_LOOP;
		} else if(smode == "triangles") {
			return GL_TRIANGLES;
		} else if(smode == "triangle_strip") {
			return GL_TRIANGLE_STRIP;
		} else if(smode == "triangle_fan") {
			return GL_TRIANGLE_FAN;
		}
		ASSERT_LOG(false, "Unexpected mode type: " << smode);
		return GL_POINTS;
	}

	GLenum get_blend_mode(variant v)
	{
		if(v.is_string()) {
			const std::string s = v.as_string();
			if(s == "zero") {
				return GL_ZERO;
			} else if(s == "one") {
				return GL_ONE;
			} else if(s == "src_color") {
				return GL_SRC_COLOR;
			} else if(s == "one_minus_src_color") {
				return GL_ONE_MINUS_SRC_COLOR;
			} else if(s == "src_alpha") {
				return GL_SRC_ALPHA;
			} else if(s == "one_minus_src_alpha") {
				return GL_ONE_MINUS_SRC_ALPHA;
			} else if(s == "dst_alpha") {
				return GL_DST_ALPHA;
			} else if(s == "one_minus_dst_alpha") {
				return GL_ONE_MINUS_DST_ALPHA;
			} 
			ASSERT_LOG(false, "Unrecognised blend mode (maybe needs adding): " << s);
		} else if(v.is_int()) {
			return v.as_int();
		}
		ASSERT_LOG(false, "Expected blend mode to be a string or integer");
		return GL_ZERO;
	}

	class get_mvp_matrix_function : public game_logic::function_expression
	{
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

	class draw_arrays_command : public game_logic::command_callable
	{
	public:
		explicit draw_arrays_command(GLenum mode, GLint first, GLsizei count)
			: mode_(mode), first_(first), count_(count)
		{}
		virtual void execute(formula_callable& ob) const
		{
			glDrawArrays(mode_, first_, count_);
		}
	private:
		GLenum mode_;
		GLint first_;
		GLsizei count_;
	};

	class draw_arrays_function : public game_logic::function_expression 
	{
	public:
		explicit draw_arrays_function(const args_list& args)
		 : function_expression("draw_arrays", args, 3, 3)
		{}
	private:
		variant execute(const game_logic::formula_callable& variables) const 
		{
			game_logic::formula::fail_if_static_context();
			GLenum mode;
			variant vmode = args()[0]->evaluate(variables);
			if(vmode.is_string()) {
				mode = convert_mode(vmode.as_string());
			} else if(vmode.is_int()) {
				mode = vmode.as_int();
			} else {
				ASSERT_LOG(false, "Unexpected type for mode argument: " << vmode.type());
			}
			return variant(new draw_arrays_command(mode, 
				args()[1]->evaluate(variables).as_int(), 
				args()[2]->evaluate(variables).as_int()));
		}
	};

	class draw_elements_command : public game_logic::command_callable
	{
	public:
		explicit draw_elements_command(GLenum mode, std::vector<GLshort>* indicies)
			: mode_(mode)
		{
			indicies_.swap(*indicies);
		}
		virtual void execute(formula_callable& ob) const
		{
			glDrawElements(mode_, indicies_.size(), GL_SHORT, &indicies_[0]);
		}
	private:
		GLenum mode_;
		std::vector<GLshort> indicies_;
	};

	class draw_elements_function : public game_logic::function_expression 
	{
	public:
		explicit draw_elements_function(const args_list& args)
		 : function_expression("draw_elements", args, 2, 2)
		{}
	private:
		variant execute(const game_logic::formula_callable& variables) const 
		{
			game_logic::formula::fail_if_static_context();
			GLenum mode;
			variant vmode = args()[0]->evaluate(variables);
			if(vmode.is_string()) {
				mode = convert_mode(vmode.as_string());
			} else if(vmode.is_int()) {
				mode = vmode.as_int();
			} else {
				ASSERT_LOG(false, "Unexpected type for mode argument: " << vmode.type());
			}
			variant ndxs = args()[1]->evaluate(variables);
			std::vector<GLshort> indicies;
			for(size_t n = 0; n < ndxs.num_elements(); ++n) {
				indicies.push_back(GLshort(ndxs[n].as_int()));
			}
			return variant(new draw_elements_command(mode, &indicies));
		}
	};

	class bind_texture_command : public game_logic::command_callable
	{
	public:
		explicit bind_texture_command(GLuint tex_id, GLuint active = 0)
			: tex_id_(tex_id), active_(active)
		{}
		virtual void execute(formula_callable& ob) const
		{
			glActiveTexture(GL_TEXTURE0 + active_);
			glBindTexture(GL_TEXTURE_2D, tex_id_);
		}
	private:
		GLuint tex_id_;
		GLuint active_;
	};

	class bind_texture_function : public game_logic::function_expression 
	{
	public:
		explicit bind_texture_function(const args_list& args)
		 : function_expression("bind_texture", args, 1, 2)
		{}
	private:
		variant execute(const game_logic::formula_callable& variables) const 
		{
			GLuint active_tex = args().size() > 1 ? args()[1]->evaluate(variables).as_int() : 0;
			return variant(new bind_texture_command(GLuint(args()[0]->evaluate(variables).as_int()), active_tex));
		}
	};

	class texture_callable : public game_logic::formula_callable
	{
	public:
		explicit texture_callable(const graphics::texture& tex): tex_(tex)
		{}
		variant get_value(const std::string& key) const
		{
			if(key == "id") {
				return variant(tex_.get_id());
			}
			return variant();
		}
	private:
		graphics::texture tex_;
	};

	class load_texture_function : public game_logic::function_expression 
	{
	public:
		explicit load_texture_function(const args_list& args)
		 : function_expression("load_texture", args, 1, 1)
		{}
	private:
		variant execute(const game_logic::formula_callable& variables) const 
		{
			const std::string filename = module::map_file(args()[0]->evaluate(variables).as_string());
			const graphics::texture tex = graphics::texture::get(filename);
			return variant(new texture_callable(tex));
		}
	};

	class blend_mode_command : public game_logic::command_callable
	{
	public:
		explicit blend_mode_command(GLenum src, GLenum dst)
			: src_(src), dst_(dst)
		{}
		virtual void execute(formula_callable& ob) const
		{
			glEnable(GL_BLEND);
			glBlendFunc(src_, dst_);
		}
	private:
		GLenum src_;
		GLenum dst_;
	};

	class blend_mode_function : public game_logic::function_expression 
	{
	public:
		explicit blend_mode_function(const args_list& args)
		 : function_expression("blend_mode", args, 2, 2)
		{}
	private:
		variant execute(const game_logic::formula_callable& variables) const 
		{
			const GLenum src = get_blend_mode(args()[0]->evaluate(variables));
			const GLenum dst = get_blend_mode(args()[1]->evaluate(variables));
			return variant(new blend_mode_command(src, dst));
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
			} else if(fn == "draw_arrays") {
				return game_logic::expression_ptr(new draw_arrays_function(args));
			} else if(fn == "draw_elements") {
				return game_logic::expression_ptr(new draw_elements_function(args));
			} else if(fn == "bind_texture") {
				return game_logic::expression_ptr(new bind_texture_function(args));
			} else if(fn == "load_texture") {
				return game_logic::expression_ptr(new load_texture_function(args));
			} else if(fn == "blend_mode") {
				return game_logic::expression_ptr(new blend_mode_function(args));
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

variant program::get_attributes_value(const std::string& key) const
{
	std::map<std::string, actives>::const_iterator it = attribs_.find(key);
	ASSERT_LOG(it != attribs_.end(), "No attribute found with name: " << key);
	return it->second.last_value;	
}

void program::set_attributes(const std::string& key, const variant& value)
{
	std::map<std::string, actives>::iterator it = attribs_.find(key);
	ASSERT_LOG(it != attribs_.end(), "No attribute found with name: " << key << ", prog: " << get());
	const actives& a = it->second;
	WRITE_LOG(a.type == GL_FLOAT || a.type == GL_FLOAT_VEC2 || a.type == GL_FLOAT_VEC3 || a.type == GL_FLOAT_VEC4, 
		"Attribute type must be float not: " << a.type);
	it->second.last_value = value;

	if(value.is_callable()) {
		boost::intrusive_ptr<game_logic::float_array_callable> f = value.try_convert<game_logic::float_array_callable>();
		boost::intrusive_ptr<game_logic::short_array_callable> s = value.try_convert<game_logic::short_array_callable>();
		if(f != NULL) {
			::glVertexAttribPointer(a.location, f->num_elements(), GL_FLOAT, GL_FALSE, 0, &(f->floats()[0]));
			//GLenum err = glGetError();
			//GLint prog, max_va;
			//glGetIntegerv(GL_CURRENT_PROGRAM, &prog);
			//glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &max_va);
			//ASSERT_LOG(err == GL_NONE, "Error in OpenGL: 0x" 
			//		<< std::hex << err << std::dec << " : " << key << " : " << a.location << " : " << a.type << " : " << f->num_elements()
			//		<< ", progs " << prog << " : " << get() << ", max vertex attribs: " << max_va);
		} else if(s != NULL) {
			::glVertexAttribPointer(a.location, s->num_elements(), GL_SHORT, GL_FALSE, 0, &(s->shorts()[0]));
			//GLenum err = glGetError();
			//ASSERT_LOG(err == GL_NONE, "Error in OpenGL: 0x" << std::hex << err << std::dec << " : " << key << " : " << a.location << " : " << a.type);
		} else {
			ASSERT_LOG(false, "Couldn't convert to float_array or short_array type: " << a.name);
		}
		::glEnableVertexAttribArray(a.location);
		//GLenum err = glGetError();
		//ASSERT_LOG(err == GL_NONE, "Error in OpenGL: 0x" << std::hex << err << std::dec << " : " << key << " : " << a.location << " : " << a.type);
		active_attributes_.push_back(a.location);
	} else {//if(a.num_elements == value.num_elements()) {
		// Probably just a constant. not an attrib array.
		if(value.num_elements() == 1) {
			ASSERT_LOG(value.is_decimal(), "Value not floating point number");
			glVertexAttrib1f(a.location, GLfloat(value.as_decimal().as_float()));
		} else if(value.num_elements() == 2) {
			ASSERT_LOG(value.is_list(), "Value not list");
			glVertexAttrib2f(a.location, 
				GLfloat(value[0].as_decimal().as_float()),
				GLfloat(value[1].as_decimal().as_float()));
		} else if(value.num_elements() == 3) {
			ASSERT_LOG(value.is_list(), "Value not list");
			glVertexAttrib3f(a.location, 
				GLfloat(value[0].as_decimal().as_float()),
				GLfloat(value[1].as_decimal().as_float()),
				GLfloat(value[2].as_decimal().as_float()));
		} else if(value.num_elements() == 4) {
			ASSERT_LOG(value.is_list(), "Value not list");
			//std::cerr << "set attribute \"" << key << "\" to " << value << std::endl;
			glVertexAttrib4f(a.location, 
				GLfloat(value[0].as_decimal().as_float()),
				GLfloat(value[1].as_decimal().as_float()),
				GLfloat(value[2].as_decimal().as_float()),
				GLfloat(value[3].as_decimal().as_float()));
			glDisableVertexAttribArray(a.location);
		} else {
			ASSERT_LOG(false, "Unrecognised attribute type: " << value.type() << " : " << a.name << " : " << a.num_elements << "," << value.num_elements());
		}
	//} else {
	//	ASSERT_LOG(false, "Unrecognised attribute type: " << value.type() << " : " << a.name << " : " << a.num_elements << "," << value.num_elements());
	}
}

void program::disable_vertex_attrib(GLint)
{
	for(size_t n = 0; n < active_attributes_.size(); ++n) {
		::glDisableVertexAttribArray(active_attributes_[n]);
	}
	active_attributes_.clear();
}

variant program::write()
{
	variant_builder res;
	res.add("program", name());
	res.add("vertex", vs_.name());
	res.add("fragment", fs_.name());
	if(stored_attributes_.is_null() == false) {
		res.add("attributes", stored_attributes_);
	}
	return res.build();
}

void program::vertex_attrib_array(GLint ndx, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* ptr)
{
	::glVertexAttribPointer(ndx, size, type, normalized, stride, ptr);
	::glEnableVertexAttribArray(ndx);
	active_attributes_.push_back(ndx);
}

void program::vertex_array(GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* ptr)
{
	if(stored_attributes_.has_key("vertex")) {
		const variant& v = stored_attributes_["vertex"];
		if(v.is_string()) {
			vertex_attrib_array(get_attribute(v.as_string()), size, type, normalized, stride, ptr);
		} else {
			ASSERT_LOG(false, "Expected vertex attribute to be string.");
		}
	} else {
		ASSERT_LOG(false, "No attribute mapping found for: 'vertex', program: " << name());
	}
}

void program::texture_array(GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* ptr)
{
	if(stored_attributes_.has_key("texcoord")) {
		const variant& v = stored_attributes_["texcoord"];
		if(v.is_string()) {
			vertex_attrib_array(get_attribute(v.as_string()), size, type, normalized, stride, ptr);
		} else {
			ASSERT_LOG(false, "Expected texcoord attribute to be string.");
		}
	} else {
		ASSERT_LOG(false, "No attribute mapping found for: 'texcoord', program: " << name());
	}
}

void program::color_array(GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* ptr)
{
	if(stored_attributes_.has_key("color")) {
		const variant& v = stored_attributes_["color"];
		if(v.is_string()) {
			vertex_attrib_array(get_attribute(v.as_string()), size, type, normalized, stride, ptr);
		} else {
			ASSERT_LOG(false, "Expected color attribute to be string.");
		}
	} else {
		ASSERT_LOG(false, "No attribute mapping found for: 'color', program: " << name());
	}
}

void program::set_fixed_attributes(const variant& node)
{
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

		std::map<std::string, gles2::program_ptr>::iterator it = shader_programs.find(program_name);
		ASSERT_LOG(it != shader_programs.end(), "Error! Something bad happened adding the shader.");
		std::cerr << "Loaded shader program: \"" << program_name << "\"(" << it->second->get() << ") from file. (" 
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
	if(prog.is_null() == false) {
		shader_programs[program_name]->set_fixed_attributes(prog);
	}
}

program_ptr program::find_program(const std::string& prog_name)
{
	std::map<std::string, gles2::program_ptr>::const_iterator it = shader_programs.find(prog_name);
	ASSERT_LOG(it != shader_programs.end(), "Shader program \"" << prog_name << "\" not found.");
	return it->second;
}

std::map<std::string, gles2::program_ptr>& program::get_shaders()
{
	return shader_programs;
}

void program::clear_shaders()
{
	shader_programs.clear();
}

///////////////////////////////////////////////////////////////////////////
// shader_program

shader_program::shader_program()
	: vars_(new game_logic::formula_variable_storage()), parent_(NULL), zorder_(-1)
{
}

shader_program::shader_program(const variant& node, entity* obj)
	: vars_(new game_logic::formula_variable_storage()), parent_(obj), zorder_(-1)
{
	configure(node, obj);
}

void shader_program::configure(const variant& node, entity* obj)
{
	ASSERT_LOG(node.is_map(), "shader attribute must be a map.");
	name_ = node["program"].as_string();
	program_object_ = program::find_program(name_);
	game_logic::formula_callable* e = this;
	ASSERT_LOG(e != NULL, "Environment was not set.");

	zorder_ = node["zorder"].as_int(-1);
	if(node.has_key("create")) {
		const variant& c = node["create"];
		if(c.is_list()) {
			for(size_t n = 0; n < c.num_elements(); ++n) {
				std::string cmd = c[n].as_string();
				create_commands_.push_back(cmd);
				ASSERT_LOG(node.has_key(cmd) == true, "No attribute found with name: " << cmd);
				create_formulas_.push_back(e->create_formula(node[cmd]));
			}
		} else if(c.is_string()) {
			// single formula stored
			create_formulas_.push_back(e->create_formula(variant(c.as_string())));
		} else {
			ASSERT_LOG(false, "create must be string or list");
		}
	}
	if(node.has_key("draw")) {
		const variant& d = node["draw"];
		if(d.is_list()) {
			for(size_t n = 0; n < d.num_elements(); ++n) {
				std::string cmd = d[n].as_string();
				draw_commands_.push_back(cmd);
				ASSERT_LOG(node.has_key(cmd) == true, "No attribute found with name: " << cmd);
				draw_formulas_.push_back(e->create_formula(node[cmd]));
			}
		} else if(d.is_string()) {
			draw_formulas_.push_back(e->create_formula(variant(d.as_string())));
		} else {
			ASSERT_LOG(false, "draw must be string or list");
		}
	}

	vars_->read(node["vars"]);

	if(obj) {
		init(obj);
	}
}

void shader_program::init(entity* obj)
{
	ASSERT_LOG(name_.empty() != true, "Configure not run, before calling init");
	game_logic::formula_callable* e = this;
	parent_ = obj;
	GLint current_program;
	glGetIntegerv(GL_CURRENT_PROGRAM, &current_program);
	glUseProgram(program_object_->get());
	for(size_t n = 0; n < create_formulas_.size(); ++n) {
		e->execute_command(create_formulas_[n]->execute(*e));
	}
	glUseProgram(current_program);
}

variant shader_program::write()
{
	variant_builder res;
	res.add("program", name());

	if(draw_commands_.size() == 0 && draw_formulas_.size() == 1) {
		// write a single formula as a string in "draw" attribute.
		res.add("draw", draw_formulas_[0]->str());
	} else {
		ASSERT_LOG(draw_commands_.size() == draw_formulas_.size(), "commands and formulas not same size");
		for(size_t n = 0; n < draw_commands_.size(); ++n) {
			res.add("draw", draw_commands_[n]);
			res.add(draw_commands_[n], draw_formulas_[n]->str());
		}
	}

	if(create_commands_.size() == 0 && create_formulas_.size() == 1) {
		// write a single formula as a string in "draw" attribute.
		res.add("create", create_formulas_[0]->str());
	} else {
		ASSERT_LOG(create_commands_.size() == create_formulas_.size(), "commands and formulas not same size");
		for(size_t n = 0; n < create_commands_.size(); ++n) {
			res.add("create", create_commands_[n]);
			res.add(create_commands_[n], create_formulas_[n]->str());
		}
	}

	if(vars_ != NULL) {
		res.add("vars", vars_->write());
	}
	if(zorder_ != -1) {
		res.add("zorder", zorder_);
	}
	return res.build();
}

void shader_program::prepare_draw()
{
//#if defined(WIN32)
//	profile::manager manager;
//#endif
	glUseProgram(program_object_->get());
	game_logic::formula_callable* e = this;
	for(size_t n = 0; n < draw_formulas_.size(); ++n) {
		e->execute_command(draw_formulas_[n]->execute(*e));
	}
}

variant shader_program::get_value(const std::string& key) const
{
	if(key == "vars") {
		return variant(vars_.get());
	} else if(key == "parent" || key == "object") {
		ASSERT_LOG(parent_ != NULL, "Tried to request parent, when value is null");
		return variant(parent_);
	}
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
	name_.clear();
	create_commands_.clear();
	draw_commands_.clear();
	create_formulas_.clear();
	draw_formulas_.clear();
}

bool shader_program::execute_command(const variant& var)
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

game_logic::formula_ptr shader_program::create_formula(const variant& v)
{
	return game_logic::formula_ptr(new game_logic::formula(v, &get_shader_symbol_table()));
}

}
