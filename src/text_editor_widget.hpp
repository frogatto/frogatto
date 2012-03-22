#ifndef TEXT_EDITOR_WIDGET_HPP_INCLUDED
#define TEXT_EDITOR_WIDGET_HPP_INCLUDED

#include <boost/intrusive_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include "scrollable_widget.hpp"

namespace gui {

class text_editor_widget : public scrollable_widget
{
public:
	text_editor_widget(int nrows, int ncols);
	~text_editor_widget();

	std::string text() const;
	void set_text(const std::string& value);

	void undo();
	void redo();

protected:
	const std::vector<std::string>& get_data() const { return text_; }

	virtual void select_token(const std::string& row, int& begin_col, int& end_col) const;

private:
	void handle_draw() const;
	bool handle_event(const SDL_Event& event, bool claimed);
	bool handle_mouse_button_down(const SDL_MouseButtonEvent& event);
	bool handle_mouse_button_up(const SDL_MouseButtonEvent& event);
	bool handle_mouse_motion(const SDL_MouseMotionEvent& event);
	bool handle_key_press(const SDL_KeyboardEvent& key);

	std::pair<int, int> mouse_position_to_row_col(int x, int y) const;

	std::pair<int, int> char_position_on_screen(int row, int col) const;

	void delete_selection();

	void on_page_up();
	void on_page_down();
	void on_move_cursor(bool auto_shift=false);

	int find_equivalent_col(int old_col, int old_row, int new_row) const;

	void on_set_yscroll(int old_pos, int new_pos);

	void refresh_scrollbar();

	virtual text_editor_widget* clone() const;
	virtual void restore(const text_editor_widget* state);

	void save_undo_state();

	bool record_op(const char* type=NULL);

	const char* last_op_type_;

	std::vector<boost::shared_ptr<text_editor_widget> > undo_, redo_;

	std::vector<std::string> text_;

	int font_size_;
	int char_width_, char_height_;

	int row_select_, col_select_;
	int row_, col_;

	int nrows_, ncols_;
	int scroll_pos_;
	
	bool has_focus_;
	bool is_dragging_;

	int last_click_at_, consecutive_clicks_;
};

}

#endif
