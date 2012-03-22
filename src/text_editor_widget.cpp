#include <boost/bind.hpp>
#include <boost/function.hpp>

#include <algorithm>

#include "graphics.hpp"

#include "clipboard.hpp"
#include "font.hpp"
#include "foreach.hpp"
#include "raster.hpp"
#include "string_utils.hpp"
#include "text_editor_widget.hpp"
#include "unit_test.hpp"

namespace gui {

namespace {

const int TabWidth = 4;
const int TabAdjust = TabWidth - 1;

std::map<char, graphics::texture> char_to_texture;

graphics::texture get_texture(char c) {
	std::map<char, graphics::texture>::iterator i = char_to_texture.find(c);
	if(i != char_to_texture.end()) {
		return i->second;
	} else {
		std::string s = "x";
		s[0] = c;
		graphics::texture t = font::render_text(s, graphics::color_white(), 12);
		char_to_texture[c] = t;
		return t;
	}
}

}

text_editor_widget::text_editor_widget(int nrows, int ncols)
  : last_op_type_(NULL),
    font_size_(12),
    char_width_(font::char_width(font_size_)),
    char_height_(font::char_height(font_size_)),
	row_select_(0), col_select_(0), row_(0), col_(0),
	nrows_(nrows), ncols_(ncols),
	scroll_pos_(0),
	has_focus_(false),
	is_dragging_(false),
	last_click_at_(-1),
	consecutive_clicks_(0),
	text_color_(255, 255, 255, 255)
{
	set_dim(char_width_*ncols_, char_height_*nrows_);
	text_.push_back("");
}

text_editor_widget::~text_editor_widget()
{
}

std::string text_editor_widget::text() const
{
	std::string result;
	foreach(const std::string& line, text_) {
		result += line;
		result += "\n";
	}

	result.resize(result.size()-1);
	return result;
}

void text_editor_widget::set_text(const std::string& value)
{
	std::string txt = value;
	txt.erase(std::remove(txt.begin(), txt.end(), '\r'), txt.end());
	text_ = util::split(txt, '\n', 0 /*don't remove empties or strip spaces*/);
	if(text_.empty()) {
		text_.push_back("");
	}

	row_select_ = col_select_ = row_ = col_ = 0;
	scroll_pos_ = 0;

	refresh_scrollbar();
	on_change();
}

void text_editor_widget::handle_draw() const
{
	graphics::color current_color(255, 255, 255, 255);
	int r = 0;
	for(int n = scroll_pos_; n < text_.size() && r < nrows_; ++n, ++r) {
		int c = 0;
		for(int m = 0; m < text_[n].size(); ++m, ++c) {
			if(c >= ncols_) {
				++r;
				c -= ncols_;
				if(r == nrows_) {
					break;
				}
			}

			if(has_focus_ && row_ == n && col_ == m && SDL_GetTicks()%500 < 350) {
				graphics::draw_rect(rect(x() + c*char_width_+1, y() + r*char_height_, 1, char_height_), graphics::color(255,255,255,255));
				current_color.set_as_current_color();
			}

			if(text_[n][m] == '\t') {
				c += TabAdjust;
				continue;
			}

			int begin_select_row = row_select_;
			int begin_select_col = col_select_;
			int end_select_row = row_;
			int end_select_col = col_;
			if(end_select_row < begin_select_row || end_select_row == begin_select_row && end_select_col < begin_select_col) {
				std::swap(begin_select_row, end_select_row);
				std::swap(begin_select_col, end_select_col);
			}

			if((n > begin_select_row || n == begin_select_row && m >= begin_select_col) &&
			   (n < end_select_row || n == end_select_row && m < end_select_col)) {
				graphics::draw_rect(rect(x() + c*char_width_, y() + r*char_height_, char_width_, char_height_), graphics::color(255, 255, 0, 128));
				current_color.set_as_current_color();
			}

			graphics::color col = get_character_color(n, m);
			if(col.value() != current_color.value()) {
				current_color = col;
				current_color.set_as_current_color();
			}
			graphics::texture t = get_texture(text_[n][m]);
			graphics::blit_texture(t, x() + c*char_width_, y() + r*char_height_);
		}

		if(has_focus_ && row_ == n && col_ >= text_[n].size() && SDL_GetTicks()%500 < 350) {
			graphics::draw_rect(rect(x() + c*char_width_+1, y() + r*char_height_, 1, char_height_), graphics::color(255,255,255,255));
			current_color.set_as_current_color();
		}
	}

	SDL_Color border_color = graphics::color_white();
	if(!has_focus_) {
		border_color.r = 128;
		border_color.g = 128;
		border_color.b = 128;
	}

	SDL_Rect border = {x()+1, y()+1, width()-2, height()-2};
	graphics::draw_hollow_rect(border, border_color);

	scrollable_widget::handle_draw();
}

bool text_editor_widget::handle_event(const SDL_Event& event, bool claimed)
{
	claimed = scrollable_widget::handle_event(event, claimed) || claimed;

	switch(event.type) {
	case SDL_KEYDOWN:
		return handle_key_press(event.key) || claimed;
	case SDL_MOUSEBUTTONDOWN:
		return handle_mouse_button_down(event.button) || claimed;
	case SDL_MOUSEBUTTONUP:
		return handle_mouse_button_up(event.button) || claimed;
	case SDL_MOUSEMOTION:
		return handle_mouse_motion(event.motion) || claimed;
	}

	return false;
}

bool text_editor_widget::handle_mouse_button_down(const SDL_MouseButtonEvent& event)
{
	record_op();
	if(event.x >= x() && event.x < x() + width() && event.y >= y() && event.y < y() + height()) {
#if defined(_WINDOWS)
		if(event.button == SDL_BUTTON_WHEELUP) {
			if(row_ > 2) {
				row_ -= 3;
				scroll_pos_ -= 3;
				if( scroll_pos_ < 0 ){ 
					scroll_pos_ = 0; 
				}
			} else {
				row_ = 0;
			}
			col_ = find_equivalent_col(col_, row_+3, row_);
			on_move_cursor();
			return true;
		} else if(event.button == SDL_BUTTON_WHEELDOWN) {
			if(row_ < text_.size()-3) {
				row_ += 3;
				scroll_pos_ += 3;
				if( scroll_pos_ > text_.size() ){ 
					scroll_pos_ = text_.size(); 
				}
			} else {
				row_ = text_.size();
			}
			col_ = find_equivalent_col(col_, row_-3, row_);
			on_move_cursor();
			return true;
		}
#endif

		SDL_EnableUNICODE(1);
		SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
		has_focus_ = true;
		std::pair<int, int> pos = mouse_position_to_row_col(event.x, event.y);
		if(pos.first != -1) {
			row_ = pos.first;
			col_ = pos.second;
			on_move_cursor();
		}

		if(last_click_at_ != -1 && SDL_GetTicks() - last_click_at_ < 500) {
			++consecutive_clicks_;

			const int nclicks = consecutive_clicks_%3;

			if(nclicks == 1) {
				row_select_ = row_;
				col_select_ = col_;
				select_token(text_[row_], col_select_, col_);
			} else if(nclicks == 2) {
				row_select_ = row_;
				col_select_ = 0;
				col_ = text_[row_].size();
			}
		} else {
			consecutive_clicks_ = 0;
		}

		last_click_at_ = SDL_GetTicks();

		is_dragging_ = true;
		return true;
	}

	is_dragging_ = false;
	has_focus_ = false;

	return false;
}

bool text_editor_widget::handle_mouse_button_up(const SDL_MouseButtonEvent& event)
{
	record_op();
	is_dragging_ = false;
	
	return false;
}

bool text_editor_widget::handle_mouse_motion(const SDL_MouseMotionEvent& event)
{
	int mousex, mousey;
	if(is_dragging_ && has_focus_ && SDL_GetMouseState(&mousex, &mousey)) {
		std::pair<int, int> pos = mouse_position_to_row_col(event.x, event.y);
		if(pos.first != -1) {
			row_ = pos.first;
			col_ = pos.second;
			on_move_cursor(true /*don't check for shift, assume it is*/);
		}
	}

	return false;
}

bool text_editor_widget::handle_key_press(const SDL_KeyboardEvent& event)
{
	if(!has_focus_) {
		return false;
	}

	if(event.keysym.sym == SDLK_z && (event.keysym.mod&KMOD_CTRL)) {
		record_op();
		undo();
		return true;
	}

	if(event.keysym.sym == SDLK_y && (event.keysym.mod&KMOD_CTRL)) {
		record_op();
		redo();
		return true;
	}

	if((event.keysym.sym == SDLK_c || event.keysym.sym == SDLK_x) && (event.keysym.mod&KMOD_CTRL)) {
		record_op();
		int begin_row = row_;
		int begin_col = col_;
		int end_row = row_select_;
		int end_col = col_select_;
		if(begin_col > text_[begin_row].size()) {
			begin_col = text_[begin_row].size();
		}

		if(end_col > text_[end_row].size()) {
			end_col = text_[end_row].size();
		}

		if(end_row < begin_row || end_row == begin_row && end_col < begin_col) {
			std::swap(begin_row, end_row);
			std::swap(begin_col, end_col);
		}


		std::string str;
		if(begin_row == end_row) {
			str = std::string(text_[begin_row].begin() + begin_col, text_[begin_row].begin() + end_col);
		} else {
			str = std::string(text_[begin_row].begin() + begin_col, text_[begin_row].end());
			while(++begin_row < end_row) {
				str += "\n" + text_[begin_row];
			}

			str += "\n" + std::string(text_[end_row].begin(), text_[end_row].begin() + end_col);
		}

		copy_to_clipboard(str, false);
		if(event.keysym.sym == SDLK_x) {
			save_undo_state();
			delete_selection();
			on_change();
		}

		return true;
	} else if(event.keysym.sym == SDLK_v && (event.keysym.mod&KMOD_CTRL)) {
		record_op();
		save_undo_state();
		delete_selection();
		std::string txt = copy_from_clipboard(false);

		std::cerr << "COPY FROM CLIPBOARD: (" << txt << ")\n";

		txt.erase(std::remove(txt.begin(), txt.end(), '\r'), txt.end());
		std::vector<std::string> lines = util::split(txt, '\n', 0 /*don't remove empties or strip spaces*/);

		if(lines.size() == 1) {
			text_[row_].insert(text_[row_].begin() + col_, lines.front().begin(), lines.front().end());
			col_ += lines.front().size();
			refresh_scrollbar();
		} else if(lines.size() >= 2) {
			text_.insert(text_.begin() + row_ + 1, lines.back() + std::string(text_[row_].begin() + col_, text_[row_].end()));
			text_[row_] = std::string(text_[row_].begin(), text_[row_].begin() + col_) + lines.front();
			text_.insert(text_.begin() + row_ + 1, lines.begin()+1, lines.end()-1);
			row_select_ = row_ = row_ + lines.size() - 1;
			col_select_ = col_ = lines.back().size();
		}

		on_change();

		return true;
	}

	if(event.keysym.mod&KMOD_CTRL) {
		record_op();
		return false;
	}

	switch(event.keysym.sym) {
	case SDLK_LEFT:
		record_op();
		if(col_ > text_[row_].size()) {
			col_ = text_[row_].size();
		}

		--col_;
		if(col_ < 0) {
			if(row_ == 0) {
				col_ = 0;
			} else {
				--row_;
				col_ = text_[row_].size();
			}
		}

		on_move_cursor();
		break;
	case SDLK_RIGHT:
		record_op();
		++col_;
		if(col_ > text_[row_].size()) {
			if(row_ == text_.size()-1) {
				--col_;
			} else if(row_ < text_.size()-1) {
				++row_;
				col_ = 0;
			} else {
				--col_;
			}
		}
		on_move_cursor();
		break;
	case SDLK_UP:
		record_op();
		if(row_ > 0) {
			--row_;
			col_ = find_equivalent_col(col_, row_+1, row_);
		}
		on_move_cursor();

		break;
	case SDLK_DOWN:
		record_op();
		if(row_ < text_.size()-1) {
			++row_;
			col_ = find_equivalent_col(col_, row_-1, row_);
		}
		on_move_cursor();

		break;
	case SDLK_PAGEUP:
		record_op();
		on_page_up();
		while(row_ > scroll_pos_ && char_position_on_screen(row_, col_).first == -1) {
			--row_;
			col_ = find_equivalent_col(col_, row_+1, row_);
		}

		if(!(SDL_GetModState()&KMOD_SHIFT)) {
			row_select_ = row_;
			col_select_ = col_;
		}
		break;
	case SDLK_PAGEDOWN:
		record_op();
		on_page_down();
		while(row_ < scroll_pos_ && char_position_on_screen(row_, col_).first == -1) {
			++row_;
			col_ = find_equivalent_col(col_, row_-1, row_);
		}

		if(!(SDL_GetModState()&KMOD_SHIFT)) {
			row_select_ = row_;
			col_select_ = col_;
		}
		break;
	case SDLK_HOME:
		record_op();
		col_ = 0;
		on_move_cursor();
		break;
	case SDLK_END:
		record_op();
		col_ = text_[row_].size();
		on_move_cursor();
		break;
	case SDLK_DELETE:
	case SDLK_BACKSPACE:
		if(record_op("delete")) {
			save_undo_state();
		}
		if(row_ == row_select_ && col_ == col_select_) {

			if(event.keysym.sym == SDLK_BACKSPACE) {
				//backspace is like delete but we move to the left first.
				if(col_ > text_[row_].size()) {
					col_ = text_[row_].size();
				}

				if(row_ == 0 && col_ == 0) {
					break;
				}

				--col_;
				if(col_ < 0) {
					--row_;
					col_ = text_[row_].size();
				}

				on_move_cursor();
			}

			if(col_ >= text_[row_].size()) {
				if(text_.size() > row_+1) {
					col_ = text_[row_].size();
					text_[row_] += text_[row_+1];
					text_.erase(text_.begin() + row_ + 1);
				}
			} else {
				text_[row_].erase(text_[row_].begin() + col_);
			}
		} else {
			delete_selection();
		}

		refresh_scrollbar();
		on_change();
		break;
	case SDLK_RETURN: {
		if(record_op("enter")) {
			save_undo_state();
		}
		if(nrows_ == 1) {
			break;
		}

		delete_selection();

		std::string new_line(text_[row_].begin() + col_, text_[row_].end());
		text_[row_].erase(text_[row_].begin() + col_, text_[row_].end());
		text_.insert(text_.begin() + row_ + 1, new_line);
		++row_;
		col_ = 0;
		row_select_ = row_;
		col_select_ = col_;

		refresh_scrollbar();
		on_change();
		
		break;
	}
	default: {
		const char c = event.keysym.unicode;
		if(util::isprint(c)) {
			if(record_op("chars")) {
				save_undo_state();
			}
			delete_selection();
			if(col_ > text_[row_].size()) {
				col_ = text_[row_].size();
			}
			text_[row_].insert(text_[row_].begin() + col_, c);
			++col_;
			row_select_ = row_;
			col_select_ = col_;
			refresh_scrollbar();
			on_change();
			return true;
		}
		return false;
	}
	}

	return true;
}

void text_editor_widget::delete_selection()
{
	if(col_ == col_select_ && row_ == row_select_) {
		return;
	}

	if(col_ > text_[row_].size()) {
		col_ = text_[row_].size();
	}

	if(col_select_ > text_[row_select_].size()) {
		col_select_ = text_[row_select_].size();
	}

	if(row_select_ < row_ || row_select_ == row_ && col_select_ < col_) {
		//make row_/col_ come first
		std::swap(row_select_, row_);
		std::swap(col_select_, col_);
	}

	if(row_select_ == row_) {
		text_[row_].erase(text_[row_].begin() + col_, text_[row_].begin() + col_select_);
	} else {
		text_[row_] = std::string(text_[row_].begin(), text_[row_].begin() + col_) + std::string(text_[row_select_].begin() + col_select_, text_[row_select_].end());

		text_.erase(text_.begin() + row_ + 1, text_.begin() + row_select_ + 1);
	}

	row_select_ = row_;
	col_select_ = col_;
}

graphics::color text_editor_widget::get_character_color(int row, int col) const
{
	return text_color_;
}

std::pair<int, int> text_editor_widget::mouse_position_to_row_col(int xpos, int ypos) const
{
	int r = 0;
	for(int n = scroll_pos_; n < text_.size() && r < nrows_; ++n, ++r) {
		int c = 0;
		bool matches_row = ypos >= y() + r*char_height_ && ypos < y() + (r+1)*char_height_;
		for(int m = 0; m < text_[n].size(); ++m, ++c) {
			if(c >= ncols_) {
				if(matches_row) {
					break;
				}
				++r;
				c -= ncols_;
				matches_row = ypos >= y() + r*char_height_ && ypos < y() + (r+1)*char_height_;
				if(r == nrows_) {
					break;
				}
			}

			const int char_size = text_[n][m] == '\t' ? TabWidth : 1;

			if(matches_row && xpos >= x() + c*char_width_ && xpos < x() + (c+char_size)*char_width_) {
				return std::pair<int, int>(n, m);
			}

			if(text_[n][m] == '\t') {
				c += TabAdjust;
				continue;
			}
		}

		if(matches_row) {
			return std::pair<int, int>(n, text_[n].size());
		}
	}

	return std::pair<int, int>(-1,-1);
}

std::pair<int, int> text_editor_widget::char_position_on_screen(int row, int col) const
{
	if(row < scroll_pos_) {
		return std::pair<int, int>(-1, -1);
	}

	int r = 0;
	for(int n = scroll_pos_; n < text_.size() && r < nrows_; ++n, ++r) {
		int c = 0;
		int m;
		for(m = 0; m < text_[n].size(); ++m, ++c) {
			if(c >= ncols_) {
				++r;
				c -= ncols_;
				if(r == nrows_) {
					break;
				}
			}

			if(row == n && col == m) {
				return std::pair<int, int>(r*char_height_, c*char_width_);
			}

			if(text_[n][m] == '\t') {
				c += TabAdjust;
				continue;
			}
		}

		if(row == n && m == text_[n].size()) {
			return std::pair<int, int>(r*char_height_, c*char_width_);
		}
	}

	return std::pair<int, int>(-1,-1);
}

void text_editor_widget::on_page_up()
{
	int leap = nrows_ - 1;
	while(scroll_pos_ > 0 && leap > 0) {
		--scroll_pos_;
		--leap;

		for(int n = int(text_[scroll_pos_].size()) - ncols_; n > 0; n -= ncols_) {
			--leap;
		}
	}

	refresh_scrollbar();
}

void text_editor_widget::on_page_down()
{
	int leap = nrows_ - 1;
	while(scroll_pos_ < int(text_.size())-2 && leap > 0) {
		++scroll_pos_;
		--leap;

		for(int n = int(text_[scroll_pos_].size()) - ncols_; n > 0; n -= ncols_) {
			--leap;
		}
	}

	refresh_scrollbar();
}

void text_editor_widget::on_move_cursor(bool auto_shift)
{
	const int start_pos = scroll_pos_;
	if(row_ < scroll_pos_) {
		scroll_pos_ = row_;
	} else {
		while(scroll_pos_ < row_ && char_position_on_screen(row_, col_).first == -1) {
			++scroll_pos_;
		}
	}

	if(start_pos != scroll_pos_) {
		refresh_scrollbar();
	}

	if(!auto_shift && !(SDL_GetModState()&KMOD_SHIFT)) {
		row_select_ = row_;
		col_select_ = col_;
	}

	scrollable_widget::set_yscroll(scroll_pos_*char_height_);
}

int text_editor_widget::find_equivalent_col(int old_col, int old_row, int new_row) const
{
	int actual_pos = old_col + std::count(text_[old_row].begin(), text_[old_row].end(), '\t')*TabAdjust;
	for(int n = 0; n < actual_pos; ++n) {
		if(n < text_[new_row].size() && text_[new_row][n] == '\t') {
			actual_pos -= TabAdjust;
		}
	}

	return actual_pos;
}

void text_editor_widget::on_set_yscroll(int old_pos, int new_pos)
{
	scroll_pos_ = new_pos/char_height_;
}

void text_editor_widget::refresh_scrollbar()
{
	int total_rows = 0;
	//See if it can all fit without a scrollbar.
	for(int n = 0; n != text_.size(); ++n) {
		const int rows = 1 + text_[n].size()/ncols_;
		total_rows += rows;
		if(total_rows > nrows_) {
			break;
		}
	}

	if(total_rows <= nrows_) {
		//no scrollbar needed.
		set_virtual_height(height());
		update_scrollbar();
		return;
	}

	set_virtual_height(text_.size()*char_height_ + height() - char_height_);
	set_scroll_step(char_height_);

	set_dim(char_width_*ncols_, char_height_*nrows_);

	set_yscroll(scroll_pos_*char_height_);

	update_scrollbar();
}

void text_editor_widget::select_token(const std::string& row, int& begin_col, int& end_col) const
{
	if(util::isalnum(row[begin_col]) || row[begin_col] == '_') {
		while(begin_col >= 0 && (util::isalnum(row[begin_col]) || row[begin_col] == '_')) {
			--begin_col;
		}

		++begin_col;

		while(end_col < row.size() && (util::isalnum(row[end_col]) || row[end_col] == '_')) {
			++end_col;
		}
	} else if(end_col < row.size()) {
		++end_col;
	}
}

text_editor_widget* text_editor_widget::clone() const
{
	text_editor_widget* result = new text_editor_widget(*this);
	result->last_op_type_ = NULL;
	return result;
}

void text_editor_widget::restore(const text_editor_widget* state)
{
	*this = *state;
}

void text_editor_widget::save_undo_state()
{
	redo_.clear();
	undo_.push_back(boost::shared_ptr<text_editor_widget>(clone()));
}

bool text_editor_widget::record_op(const char* type)
{
	if(type == NULL || type != last_op_type_) {
		last_op_type_ = type;
		return true;
	} else {
		return false;
	}
}

void text_editor_widget::undo()
{
	if(undo_.empty()) {
		return;
	}

	std::vector<boost::shared_ptr<text_editor_widget> > redo_state = redo_;
	save_undo_state();
	redo_state.push_back(undo_.back());
	undo_.pop_back();

	//Save the state before restoring it so it doesn't get cleaned up
	//while we're in the middle of the restore call.
	boost::shared_ptr<text_editor_widget> state = undo_.back();
	restore(state.get());

	redo_ = redo_state;

	on_change();
}

void text_editor_widget::redo()
{
	if(redo_.empty()) {
		return;
	}

	std::vector<boost::shared_ptr<text_editor_widget> > redo_state = redo_;
	redo_state.pop_back();

	//Save the state before restoring it so it doesn't get cleaned up
	//while we're in the middle of the restore call.
	boost::shared_ptr<text_editor_widget> state = redo_.back();
	restore(state.get());

	redo_ = redo_state;

	on_change();
}

}

#include "code_editor_widget.hpp"
#include "dialog.hpp"
#include "filesystem.hpp"

UTILITY(textedit)
{
	using namespace gui;
	if(args.size() != 1) {
		std::cerr << "textedit usage: <filename>\n";
		return;
	}

	std::string file = args[0];
	std::string contents = sys::read_file(file);
	if(contents.empty()) {
		std::cerr << "Could not read file (" << file << ")\n";
		return;
	}

	text_editor_widget* entry = new text_editor_widget(1, 80);

	text_editor_widget* editor = new code_editor_widget(30, 80);
	editor->set_text(contents);

	dialog d(0, 0, graphics::screen_width(), graphics::screen_height());
	d.add_widget(widget_ptr(entry), 10, 10);
	d.add_widget(widget_ptr(editor), 10, 30);
	d.show_modal();

}

