#ifndef CODE_EDITOR_WIDGET_HPP_INCLUDED
#define CODE_EDITOR_WIDGET_HPP_INCLUDED

#include <map>
#include <vector>

#include "text_editor_widget.hpp"

namespace gui {

class code_editor_widget : public text_editor_widget
{
public:
	code_editor_widget(int nrows, int ncols);
private:
	void select_token(const std::string& row, int& begin_row, int& end_row, int& begin_col, int& end_col) const;
	void on_change();
	graphics::color get_character_color(int row, int col) const;

	std::vector<std::vector<graphics::color> > colors_;

	//maps a location (a bracket or comma) to matching locations.
	std::map<std::pair<int, int>, std::vector<std::pair<int, int> > > bracket_match_;
};

}

#endif
