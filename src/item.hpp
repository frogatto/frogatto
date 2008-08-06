#ifndef ITEM_HPP_INCLUDED
#define ITEM_HPP_INCLUDED

#include "boost/intrusive_ptr.hpp"

#include "formula_callable.hpp"
#include "item_type.hpp"
#include "wml_node_fwd.hpp"

class level;
class item;
typedef boost::intrusive_ptr<item> item_ptr;
typedef boost::intrusive_ptr<const item> const_item_ptr;

class item : public game_logic::formula_callable
{
public:
	explicit item(wml::const_node_ptr node);
	wml::node_ptr write() const;

	void process(level& lvl);
	bool destroyed() const;
	void draw() const;

	int x() const { return x_; }
	int y() const { return y_; }

private:
	variant get_value(const std::string& key) const;
	void get_inputs(std::vector<game_logic::formula_input>* inputs) const;
	const_item_type_ptr type_;
	int x_, y_;
	int time_in_frame_;
	bool touched_;
};

#endif
