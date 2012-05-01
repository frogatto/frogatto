#ifndef CODE_EDITOR_WIDGET_HPP_INCLUDED
#define CODE_EDITOR_WIDGET_HPP_INCLUDED

#include <map>
#include <vector>

#include "json_tokenizer.hpp"
#include "slider.hpp"
#include "text_editor_widget.hpp"

namespace gui {

class code_editor_widget : public text_editor_widget
{
public:
	code_editor_widget(int width, int height);
	void on_slider_move(float value);

	const std::string& current_text() const { return current_text_; }
	struct ObjectInfo {
		int begin, end;
		variant obj;
		std::vector<json::Token> tokens;
	};

	ObjectInfo get_current_object() const;
	void set_highlight_current_object(bool value);

	//modifies the currently selected object to be equal to this new value.
	void modify_current_object(variant new_obj);

private:
	ObjectInfo get_object_at(int row, int col) const;

	virtual void handle_draw() const;
	virtual bool handle_event(const SDL_Event& event, bool claimed);
	void select_token(const std::string& row, int& begin_row, int& end_row, int& begin_col, int& end_col);
	void on_change();
	void on_move_cursor(bool auto_shift=false);
	graphics::color get_character_color(int row, int col) const;

	std::vector<std::vector<graphics::color> > colors_;

	//maps a location (a bracket or comma) to matching locations.
	std::map<std::pair<int, int>, std::vector<std::pair<int, int> > > bracket_match_;

	mutable slider_ptr slider_;
	int row_slider_, begin_col_slider_, end_col_slider_;
	bool slider_decimal_;
	int slider_magnitude_;

	void generate_tokens();
	std::string current_text_;
	variant current_obj_;
	std::vector<json::Token> tokens_;
};

}

#endif
