#pragma once
#ifndef SHADERS_HPP_INCLUDED
#define SHADERS_HPP_INCLUDED

#include <map>
#include <vector>
#include <boost/intrusive_ptr.hpp>

#include "asserts.hpp"
#include "formula_callable.hpp"

namespace gles2
{

class shader 
{
public:
	shader() : type_(0), shader_(0)
	{}
	explicit shader(GLenum type, const std::string& name, const std::string& code);
	GLuint get() const { return shader_; }
	std::string name() const { return name_; }
protected:
	bool compile(const std::string& code);
private:
	GLenum type_;
	GLuint shader_;
	std::string name_;
};

struct actives
{
	// Name of variable.
	std::string name;
	// type of the uniform/attribute variable
	GLenum type;
	// If an array type, this is the maximum number of array elements used 
	// in the program. Value is 1 if type is not an array type.
	GLsizei num_elements;
	// Location of the active uniform/attribute
	GLint location;
};

class program;
typedef boost::intrusive_ptr<program> program_ptr;
typedef boost::intrusive_ptr<const program> const_program_ptr;

class program : public game_logic::formula_callable
{
public:
	program();
	explicit program(const std::string& name, const shader& vs, const shader& fs);
	virtual ~program()
	{}
	void init(const std::string& name, const shader& vs, const shader& fs);
	GLuint get() const { return object_; }
	GLuint get_attribute(const std::string& attr) const;
	GLuint get_uniform(const std::string& attr) const;
	std::string name() const { return name_; }
	game_logic::formula_ptr create_formula(const variant& v);
	bool execute_command(const variant& var);
	void set_uniform(const std::string& key, const variant& value);
	game_logic::formula_callable* get_environment() { return environ_; }

	variant write();

	void vertex_array(GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* ptr);
	void texture_array(GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* ptr);
	void color_array(GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* ptr);

	void vertex_attrib_array(GLint ndx, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* ptr);
	void disable_vertex_attrib(GLint);

	static void load_shaders(const std::string& shader_data);
	static program_ptr find_program(const std::string& prog_name);
	static void add_shader(const std::string& program_name, 
		const shader& v_shader, 
		const shader& f_shader, 
		const variant& prog);
protected:
	bool link();
	bool queryUniforms();
	bool queryAttributes();

	virtual variant get_value(const std::string& key) const;
	virtual void set_value(const std::string& key, const variant& value);

	void set_attributes(const variant& node);
private:
	game_logic::formula_callable* environ_;
	std::string name_;
	shader vs_;
	shader fs_;
	GLuint object_;
	std::map<std::string, actives> attribs_;
	std::map<std::string, actives> uniforms_;

	// Same cached variables to speed things up.
	GLint vtx_coord_;
	GLint tex_coord_[2];
	GLint col_coord_;
	variant stored_attributes_;
	friend class shader_program;
};

class shader_program : public game_logic::formula_callable
{
public:
	shader_program();
	explicit shader_program(const variant& node);
	virtual ~shader_program()
	{}
	void init(const variant& node);

	void prepare_draw();
	program_ptr shader() const;
	std::string name() const { return name_; }

	void clear();
	variant write();
protected:
	virtual variant get_value(const std::string& key) const;
	virtual void set_value(const std::string& key, const variant& value);
private:
	std::string name_;
	program_ptr program_object_;

	game_logic::formula_ptr create_;
	game_logic::formula_ptr draw_;
};

typedef boost::intrusive_ptr<shader_program> shader_ptr;
typedef boost::intrusive_ptr<const shader_program> const_shader_ptr;

}

#endif
