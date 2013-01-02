#pragma once
#ifndef SHADERS_HPP_INCLUDED
#define SHADERS_HPP_INCLUDED

#include <map>
#include <vector>
#include <boost/intrusive_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include "asserts.hpp"
#include "entity_fwd.hpp"
#include "formula_callable.hpp"
#include "formula_variable_storage.hpp"

namespace gles2
{

class shader 
{
public:
	static void set_runtime_error(const std::string& msg);
	static std::string get_and_clear_runtime_error();

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
	// Last value
	variant last_value;
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
	void set_uniform(const std::map<std::string,actives>::iterator& it, const variant& value);
	void set_uniform_or_defer(const std::string& key, const variant& value);
	variant get_uniform_value(const std::string& key) const;
	void set_attributes(const std::string& key, const variant& value);
	variant get_attributes_value(const std::string& key) const;
	game_logic::formula_callable* get_environment() { return environ_; }
	void set_deferred_uniforms();

	virtual variant write();

	void vertex_attrib_array(GLint ndx, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* ptr);

	virtual void vertex_array(GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* ptr);
	virtual void texture_array(GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* ptr);
	virtual void color_array(GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* ptr);
	virtual void set_fixed_attributes(const variant& node);

	void disable_vertex_attrib(GLint);

	static void load_shaders(const std::string& shader_data);
	static program_ptr find_program(const std::string& prog_name);
	static void add_shader(const std::string& program_name, 
		const shader& v_shader, 
		const shader& f_shader,
		const variant& prog);
	static std::map<std::string, gles2::program_ptr>& get_shaders();
	static void clear_shaders();
protected:
	bool link();
	bool queryUniforms();
	bool queryAttributes();

	virtual variant get_value(const std::string& key) const;
	virtual void set_value(const std::string& key, const variant& value);

	std::vector<GLint> active_attributes_;
	variant stored_attributes_;
private:
	game_logic::formula_callable* environ_;
	std::string name_;
	shader vs_;
	shader fs_;
	GLuint object_;
	std::map<std::string, actives> attribs_;
	std::map<std::string, actives> uniforms_;

	std::vector<std::string> uniforms_to_update_;

	friend class shader_program;
};

class shader_program : public game_logic::formula_callable
{
public:
	shader_program();
	explicit shader_program(const variant& node, entity* obj = NULL);
	explicit shader_program(const std::string& program_name);
	virtual ~shader_program()
	{}
	void configure(const variant& node, entity* obj = NULL);
	void init(entity* obj);
	game_logic::formula_ptr create_formula(const variant& v);
	bool execute_command(const variant& var);
	int zorder() const { return zorder_; }

	void prepare_draw();
	program_ptr shader() const;
	const std::string& name() const { return name_; }
	entity* parent() const { return parent_; }
	void set_parent(entity* obj) { parent_ = obj; }

	game_logic::formula_callable* vars() { return vars_.get(); }
	const game_logic::formula_callable* vars() const { return vars_.get(); }

	void clear();
	variant write();
protected:
	virtual variant get_value(const std::string& key) const;
	virtual void set_value(const std::string& key, const variant& value);
private:
	std::string name_;
	program_ptr program_object_;

	game_logic::formula_variable_storage_ptr vars_;

	std::vector<std::string> create_commands_;
	std::vector<std::string> draw_commands_;
	std::vector<game_logic::formula_ptr> create_formulas_;
	std::vector<game_logic::formula_ptr> draw_formulas_;

	// fake zorder value
	int zorder_;

	entity* parent_;
};

typedef boost::intrusive_ptr<shader_program> shader_ptr;
typedef boost::intrusive_ptr<const shader_program> const_shader_ptr;

}

#endif
