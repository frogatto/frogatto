#ifndef CODE_EDITOR_WIDGET_HPP_INCLUDED
#define CODE_EDITOR_WIDGET_HPP_INCLUDED

#include <map>
#include <vector>

#include "slider.hpp"
#include "text_editor_widget.hpp"

namespace gui {

class code_editor_widget : public text_editor_widget
{
public:
	code_editor_widget(int width, int height);
	void on_slider_move(float value);
private:
	virtual void handle_draw() const;
	virtual bool handle_event(const SDL_Event& event, bool claimed);
	void select_token(const std::string& row, int& begin_row, int& end_row, int& begin_col, int& end_col);
	void on_change();
	graphics::color get_character_color(int row, int col) const;

	std::vector<std::vector<graphics::color> > colors_;

	//maps a location (a bracket or comma) to matching locations.
	std::map<std::pair<int, int>, std::vector<std::pair<int, int> > > bracket_match_;

	mutable slider_ptr slider_;
	int row_slider_, begin_col_slider_, end_col_slider_;
	bool slider_decimal_;
	int slider_magnitude_;
};

}

#endif
