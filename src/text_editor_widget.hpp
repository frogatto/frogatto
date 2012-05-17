#ifndef TEXT_EDITOR_WIDGET_HPP_INCLUDED
#define TEXT_EDITOR_WIDGET_HPP_INCLUDED

#include <boost/intrusive_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include "color_utils.hpp"
#include "scrollable_widget.hpp"

namespace gui {

class text_editor_widget : public scrollable_widget
{
public:
	text_editor_widget(int width, int height=0);
	~text_editor_widget();

	std::string text() const;
	void set_text(const std::string& value, bool reset_cursor=true);

	int get_font_size() const { return font_size_; }
	void set_font_size(int font_size);
	void change_font_size(int amount);

	virtual void set_dim(int w, int h);

	void undo();
	void redo();

	struct Loc {
		Loc(int r, int c) : row(r), col(c)
		{}
		bool operator==(const Loc& o) const { return row == o.row && col == o.col; }
		bool operator!=(const Loc& o) const { return !(*this == o); }
		bool operator<(const Loc& o) const { return row < o.row || row == o.row && col < o.col; }
		bool operator>(const Loc& o) const { return o < *this; }
		bool operator<=(const Loc& o) const { return operator==(o) || operator<(o); }
		bool operator>=(const Loc& o) const { return o <= *this; }
		int row, col;
	};

	const std::vector<std::string>& get_data() const { return text_; }

	void set_search(const std::string& term);
	void next_search_match();

	void set_on_change_handler(boost::function<void()> fn) { on_change_ = fn; }
	void set_on_move_cursor_handler(boost::function<void()> fn) { on_move_cursor_ = fn; }
	void set_on_enter_handler(boost::function<void()> fn) { on_enter_ = fn; }
	void set_on_begin_enter_handler(boost::function<bool()> fn) { on_begin_enter_ = fn; }
	void set_on_tab_handler(boost::function<void()> fn) { on_tab_ = fn; }

	bool has_focus() const { return has_focus_; }
	void set_focus(bool value);

	int cursor_row() const { return cursor_.row; }
	int cursor_col() const { return cursor_.col; }

	void set_cursor(int row, int col);

	//convert a row/col cursor position to a position within the text()
	//string that is returned.
	int row_col_to_text_pos(int row, int col) const;

	std::pair<int,int> text_pos_to_row_col(int pos) const;

	void set_highlight_lines(int begin, int end);
	void clear_highlight_lines();

	std::pair<int, int> char_position_on_screen(int row, int col) const;

	void set_row_contents(int row, const std::string& value);

protected:
	virtual void select_token(const std::string& row, int& begin_row, int& end_row, int& begin_col, int& end_col);

	virtual void on_change();

	void handle_draw() const;
	bool handle_event(const SDL_Event& event, bool claimed);

	void save_undo_state();
	bool record_op(const char* type=NULL);

	std::pair<int, int> mouse_position_to_row_col(int x, int y) const;

	virtual void on_move_cursor(bool auto_shift=false);

private:
	bool handle_mouse_button_down(const SDL_MouseButtonEvent& event);
	bool handle_mouse_button_up(const SDL_MouseButtonEvent& event);
	bool handle_mouse_motion(const SDL_MouseMotionEvent& event);
	bool handle_key_press(const SDL_KeyboardEvent& key);

	virtual graphics::color get_character_color(int row, int col) const;

	void delete_selection();

	void on_page_up();
	void on_page_down();

	int find_equivalent_col(int old_col, int old_row, int new_row) const;

	void on_set_yscroll(int old_pos, int new_pos);

	void refresh_scrollbar();

	virtual text_editor_widget* clone() const;
	virtual void restore(const text_editor_widget* state);

	const char* last_op_type_;

	std::vector<boost::shared_ptr<text_editor_widget> > undo_, redo_;

	std::vector<std::string> text_;

	int font_size_;
	int char_width_, char_height_;

	Loc select_, cursor_;

	int nrows_, ncols_;
	int scroll_pos_;

	int begin_highlight_line_, end_highlight_line_;
	
	bool has_focus_;
	bool is_dragging_;

	int last_click_at_, consecutive_clicks_;

	graphics::color text_color_;

	std::string search_;
	std::vector<std::pair<Loc, Loc> > search_matches_;
	void calculate_search_matches();

	void truncate_col_position();

	boost::function<void()> on_change_, on_move_cursor_, on_enter_, on_tab_;
	boost::function<bool()> on_begin_enter_;
};

}

#endif
