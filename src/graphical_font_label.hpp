#ifndef GRAPHICAL_FONT_LABEL_HPP_INCLUDED
#define GRAPHICAL_FONT_LABEL_HPP_INCLUDED

#include "graphical_font.hpp"
#include "widget.hpp"

namespace gui {

class graphical_font_label : public widget
{
public:
	graphical_font_label(const std::string& text, const std::string& font, int size=1);
	graphical_font_label(const variant& v, const game_logic::formula_callable_ptr& e);

	void set_text(const std::string& text);
	void reset_text_dimensions();
	
	void set_value(const std::string& key, const variant& v);
	variant get_value(const std::string& key) const;
private:
	void handle_draw() const;

	std::string text_;
	const_graphical_font_ptr font_;
	int size_;
};

typedef boost::intrusive_ptr<graphical_font_label> graphical_font_label_ptr;

}

#endif
