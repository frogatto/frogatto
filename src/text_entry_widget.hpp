#ifndef TEXT_ENTRY_WIDGET_HPP_INCLUDED
#define TEXT_ENTRY_WIDGET_HPP_INCLUDED

#include <string>

#include "graphical_font.hpp"
#include "widget.hpp"

namespace gui {

class text_entry_widget : public widget
{
public:
	text_entry_widget();

	const std::string& text() const;
	void set_text(const std::string& value);

	void set_font(const std::string& font_name);
private:
	void handle_draw() const;
	bool handle_event(const SDL_Event& event, bool claimed);

	std::string text_;
	int scroll_;
	int cursor_;

	const_graphical_font_ptr font_;
};

typedef boost::intrusive_ptr<text_entry_widget> text_entry_widget_ptr;

}

#endif
