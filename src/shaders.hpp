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
	void configure(const variant& node, game_logic::formula_callable* e);
	game_logic::formula_ptr create_formula(const variant& v);
	bool execute_command(const variant& var);
	void set_uniform(const std::string& key, const variant& value);
	void prepare_draw();

	void vertex_array(GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* ptr);
	void texture_array(GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* ptr);
	void color_array(GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* ptr);

	void vertex_attrib_array(GLint ndx, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* ptr);
	void program::disable_vertex_attrib(GLint);
protected:
	bool link();
	bool queryUniforms();
	bool queryAttributes();

	virtual variant get_value(const std::string& key) const;
	virtual void set_value(const std::string& key, const variant& value);
private:
	game_logic::formula_callable* environ_;
	std::string name_;
	shader vs_;
	shader fs_;
	GLuint object_;
	std::map<std::string, actives> attribs_;
	std::map<std::string, actives> uniforms_;
	game_logic::formula_ptr create_;
	game_logic::formula_ptr draw_;

	// Same cached variables to speed things up.
	GLint vtx_coord_;
	GLint tex_coord_[2];
	GLint col_coord_;
};

typedef boost::intrusive_ptr<program> program_ptr;
typedef boost::intrusive_ptr<const program> const_program_ptr;

program_ptr find_program(const std::string& prog_name);

}

#endif