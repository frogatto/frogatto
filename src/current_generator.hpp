#ifndef CURRENT_GENERATOR_HPP_INCLUDED
#define CURRENT_GENERATOR_HPP_INCLUDED

#include <boost/intrusive_ptr.hpp>

#include "formula_callable.hpp"
#include "wml_node_fwd.hpp"

typedef boost::intrusive_ptr<class current_generator> current_generator_ptr;

class current_generator : public game_logic::formula_callable
{
public:
	static current_generator_ptr create(wml::const_node_ptr node);

	virtual ~current_generator();

	virtual void generate(int center_x, int center_y, int target_x, int target_y, int* velocity_x, int* velocity_y) = 0;
	virtual wml::node_ptr write() const = 0;
private:
	virtual variant get_value(const std::string& key) const;
};

class radial_current_generator : public current_generator
{
public:
	radial_current_generator(int intensity, int radius);
	explicit radial_current_generator(wml::const_node_ptr node);
	virtual ~radial_current_generator() {}

	virtual void generate(int center_x, int center_y, int target_x, int target_y, int* velocity_x, int* velocity_y);
	virtual wml::node_ptr write() const;
private:
	int intensity_;
	int radius_;
};

#endif
