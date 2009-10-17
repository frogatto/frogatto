#ifndef STATUS_GUI_HPP_INCLUDED
#define STATUS_GUI_HPP_INCLUDED

#include <string>
#include <vector>

#include <boost/intrusive_ptr.hpp>

#include "entity_fwd.hpp"
#include "formula_callable.hpp"
#include "variant.hpp"

class status_gui;
typedef boost::intrusive_ptr<status_gui> status_gui_ptr;
typedef boost::intrusive_ptr<const status_gui> const_status_gui_ptr;

class status_gui : public game_logic::formula_callable
{
public:
	variant get_value(const std::string& key) const;
	void set_value(const std::string& key, const variant& value);

	void process();
	void draw() const;
private:
	std::vector<entity_ptr> objects_;
};

#endif
