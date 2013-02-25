#ifndef RICH_TEXT_LABEL_HPP_INCLUDED
#define RICH_TEXT_LABEL_HPP_INCLUDED

#include <string>
#include <vector>

#include "formula_callable.hpp"
#include "scrollable_widget.hpp"
#include "widget.hpp"

namespace gui
{

class rich_text_label : public scrollable_widget
{
public:
	rich_text_label(const variant& v, game_logic::formula_callable* e);
private:

	void handle_process();
	void handle_draw() const;
	bool handle_event(const SDL_Event& event, bool claimed);

	variant get_value(const std::string& key) const;
	void set_value(const std::string& key, const variant& v);

	std::vector<std::vector<widget_ptr> > children_;
};

}

#endif
