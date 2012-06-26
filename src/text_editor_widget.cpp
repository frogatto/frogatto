#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/regex.hpp>
#include <boost/scoped_ptr.hpp>

#include <algorithm>

#include "asserts.hpp"
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

const int BorderSize = 3;
const int TabWidth = 4;
const int TabAdjust = TabWidth - 1;

typedef boost::shared_ptr<graphics::texture> char_texture_ptr;
std::vector<char_texture_ptr> char_textures;

struct CharArea {
	GLfloat x1, y1, x2, y2;
};

std::map<int, std::map<char, CharArea> > all_char_to_area;

const CharArea& get_char_area(int font_size, char c)
{
	std::map<char, CharArea>& char_to_area = all_char_to_area[font_size];
	std::map<char, CharArea>::const_iterator i = char_to_area.find(c);
	if(i != char_to_area.end()) {
		return i->second;
	}

	const CharArea& result = char_to_area[c];

	const int char_width = font::char_width(font_size);
	const int char_height = font::char_height(font_size);

	std::string str;
	int row = 0, col = 0;
	int nchars = 0;
	for(std::map<char, CharArea>::iterator i = char_to_area.begin();
	    i != char_to_area.end(); ++i) {
		str.push_back(i->first);
	
		CharArea area = {col*char_width, row*char_height, (col+1)*char_width, (row+1)*char_height};

		char_to_area[i->first] = area;

		++col;
		if(col == 128) {
			str += "\n";
			col = 0;
			++row;
		}
	}

	char_texture_ptr& char_texture = char_textures[font_size];
	char_texture.reset(new graphics::texture(font::render_text(str, graphics::color_white(), font_size)));

	for(std::map<char, CharArea>::iterator i = char_to_area.begin();
	    i != char_to_area.end(); ++i) {
		CharArea& area = i->second;
		area.x1 = char_texture->translate_coord_x(area.x1/GLfloat(char_texture->width()));
		area.x2 = char_texture->translate_coord_x(area.x2/GLfloat(char_texture->width()));
		area.y1 = char_texture->translate_coord_y(area.y1/GLfloat(char_texture->height()));
		area.y2 = char_texture->translate_coord_y(area.y2/GLfloat(char_texture->height()));
	}

	return result;
}

void init_char_area(size_t font_size)
{
	if(char_textures.size() <= font_size) {
		char_textures.resize(font_size+1);
	}

	if(char_textures[font_size].get()) {
		return;
	}

	std::map<char, CharArea>& char_to_area = all_char_to_area[font_size];
	for(char c = 1; c < 127; ++c) {
		if(util::c_isprint(c) && c != 'a') {
			char_to_area[c] = CharArea();
		}
	}

	get_char_area(font_size, 'a');
	ASSERT_LOG(char_textures[font_size].get(), "DID NOT INIT CHAR TEXTURE\n");
}

}

text_editor_widget::text_editor_widget(int width, int height)
  : last_op_type_(NULL),
    font_size_(14),
    char_width_(font::char_width(font_size_)),
    char_height_(font::char_height(font_size_)),
	select_(0,0), cursor_(0,0),
	nrows_((height - BorderSize*2)/char_height_),
	ncols_((width - 20 - BorderSize*2)/char_width_),
	scroll_pos_(0),
	begin_highlight_line_(-1), end_highlight_line_(-1),
	has_focus_(false), 
	is_dragging_(false),
	begin_enter_return_(true),
	last_click_at_(-1),
	consecutive_clicks_(0),
	text_color_(255, 255, 255, 255)
{
	set_environment();
	if(height == 0) {
		height = char_height_ + BorderSize*2;
		nrows_ = 1;
		ncols_ = width/char_width_;
		widget::set_dim(width, height);
	} else {
		widget::set_dim(width - 20, height);
	}

	text_.push_back("");

	init_clipboard();
}

text_editor_widget::text_editor_widget(const variant& v, game_logic::formula_callable* e)
	: widget(v,e), scrollable_widget(v,e), last_op_type_(NULL), font_size_(14), 
	select_(0,0), cursor_(0,0), scroll_pos_(0),
	begin_highlight_line_(-1), end_highlight_line_(-1),
	has_focus_(false), 
	is_dragging_(false),
	begin_enter_return_(true),
	last_click_at_(-1),
	consecutive_clicks_(0),
	text_color_(255, 255, 255, 255)
{
	ASSERT_LOG(get_environment() != 0, "You must specify a callable environment");
	int width = v.has_key("width") ? v["width"].as_int() : 0;
	int height = v.has_key("height") ? v["height"].as_int() : 0;
	if(v.has_key("font_size")) { 
		font_size_ = v["font_size"].as_int(); 
	}
	if(v.has_key("color")) {
		text_color_ = graphics::color(v["color"]);
	}

	if(v.has_key("on_change")) {
		on_change_ = boost::bind(&text_editor_widget::change_delegate, this);
		ffl_on_change_ = get_environment()->create_formula(v["on_change"]);
	}
	if(v.has_key("on_move_cursor")) {
		on_move_cursor_ = boost::bind(&text_editor_widget::move_cursor_delegate, this);
		ffl_on_move_cursor_ = get_environment()->create_formula(v["on_move_cursor"]);
	}
	if(v.has_key("on_enter")) {
		on_enter_ = boost::bind(&text_editor_widget::enter_delegate, this);
		ffl_on_enter_ = get_environment()->create_formula(v["on_enter"]);
	}
	if(v.has_key("on_tab")) {
		on_tab_ = boost::bind(&text_editor_widget::tab_delegate, this);
		ffl_on_tab_ = get_environment()->create_formula(v["on_tab"]);
	}
	if(v.has_key("on_escape")) {
		on_escape_ = boost::bind(&text_editor_widget::escape_delegate, this);
		ffl_on_escape_ = get_environment()->create_formula(v["on_escape"]);
	}
	if(v.has_key("on_begin_enter")) {
		on_begin_enter_ = boost::bind(&text_editor_widget::begin_enter_delegate, this);
		ffl_on_begin_enter_ = get_environment()->create_formula(v["on_begin_enter"]);
	}
	if(v.has_key("on_change_focus")) {
		on_change_focus_ = boost::bind(&text_editor_widget::change_focus_delgate, this, _1);
		ffl_on_change_focus_ = get_environment()->create_formula(v["on_change_focus"]);
	}

	char_width_= font::char_width(font_size_);
    char_height_ = font::char_height(font_size_);
	nrows_ = (height - BorderSize*2)/char_height_;
	ncols_ = (width - 20 - BorderSize*2)/char_width_;

	if(height == 0) {
		height = char_height_ + BorderSize*2;
		nrows_ = 1;
		ncols_ = width/char_width_;
		widget::set_dim(width, height);
	} else {
		widget::set_dim(width - 20, height);
	}

	text_.push_back("");

	init_clipboard();
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

void text_editor_widget::set_row_contents(int row, const std::string& value)
{
	ASSERT_LOG(row >= 0 && size_t(row) < text_.size(), "ILLEGAL ROW SET: " << row << " / " << text_.size());
	text_[row] = value;
	refresh_scrollbar();
	on_change();
}

void text_editor_widget::set_text(const std::string& value, bool reset_cursor)
{
	std::string txt = value;
	txt.erase(std::remove(txt.begin(), txt.end(), '\r'), txt.end());
	text_ = util::split(txt, '\n', 0 /*don't remove empties or strip spaces*/);
	if(text_.empty()) {
		text_.push_back("");
	}

	if(reset_cursor) {
		select_ = cursor_ = Loc(0,0);
		scroll_pos_ = 0;
	} else {
		if(select_.row >= text_.size()) {
			select_.row = text_.size() - 1;
		}

		if(cursor_.row >= text_.size()) {
			cursor_.row = text_.size() - 1;
		}
	}

	refresh_scrollbar();
	on_change();
}

void text_editor_widget::set_font_size(int font_size)
{
	if(font_size < 6) {
		font_size = 6;
	} else if(font_size > 28) {
		font_size = 28;
	}

	font_size_ = font_size;

    char_width_ = font::char_width(font_size_);
    char_height_ = font::char_height(font_size_);
	nrows_ = (height() - BorderSize*2)/char_height_;
	ncols_ = (width() - BorderSize*2)/char_width_;

	refresh_scrollbar();
}

void text_editor_widget::change_font_size(int amount)
{
	set_font_size(font_size_ + amount);
}

void text_editor_widget::set_dim(int w, int h)
{
	widget::set_dim(w - 20, h);

	nrows_ = (height() - BorderSize*2)/char_height_;
	ncols_ = (width() - BorderSize*2)/char_width_;

	refresh_scrollbar();
}

namespace {
struct RectDraw {
	rect area;
	graphics::color col;

	bool merge(RectDraw& o) {
		if(o.col.value() != col.value()) {
			return false;
		}

		if(o.area.y() != area.y() || o.area.x() > area.x() + area.w()) {
			return false;
		}

		area = rect(area.x(), area.y(), area.w() + o.area.w(), area.h());
		return true;
	}
};
}

void text_editor_widget::handle_draw() const
{
	init_char_area(font_size_);

	std::vector<RectDraw> rects;
	std::map<uint32_t, graphics::blit_queue> chars;

	int begin_build = SDL_GetTicks();

	const int xpos = x() + BorderSize;
	const int ypos = y() + BorderSize;

	int r = 0;
	for(int n = scroll_pos_; n < text_.size() && r < nrows_; ++n, ++r) {
		if(n >= begin_highlight_line_ && n <= end_highlight_line_) {
			RectDraw rect_draw = { rect(xpos, ypos + r*char_height_, width(), char_height_), graphics::color(255, 255, 255, 32) };
			rects.push_back(rect_draw);
		}

		int c = 0;
		std::vector<std::pair<Loc, Loc> >::const_iterator search_itor = std::lower_bound(search_matches_.begin(), search_matches_.end(), std::pair<Loc,Loc>(Loc(n,0),Loc(n,0)));
		for(int m = 0; m < text_[n].size(); ++m, ++c) {
			if(c >= ncols_) {
				++r;
				c -= ncols_;
				if(r == nrows_) {
					break;
				}
			}

			const int char_size = text_[n][m] == '\t' ? 4 : 1;
			Loc pos(n, m);

			Loc begin_select = select_;
			Loc end_select = cursor_;
			if(end_select < begin_select) {
				std::swap(begin_select, end_select);
			}

			graphics::color col = get_character_color(n, m);

			if(pos >= begin_select && pos < end_select) {
				RectDraw rect_draw = { rect(xpos + c*char_width_, ypos + r*char_height_, char_width_*char_size, char_height_), col };

				if(rects.empty() || !rects.back().merge(rect_draw)) {
					rects.push_back(rect_draw);
				}

				col = graphics::color(0,0,0,255);
			} else {
				for(std::vector<std::pair<Loc,Loc> >::const_iterator i = search_itor; i != search_matches_.end() && i->first <= pos; ++i) {
					if(pos >= i->first && pos < i->second) {
						RectDraw rect_draw = { rect(xpos + c*char_width_, ypos + r*char_height_, char_width_*char_size, char_height_), graphics::color(255,255,0,128) };
						if(rects.empty() || !rects.back().merge(rect_draw)) {
							rects.push_back(rect_draw);
						}

						col = graphics::color(0,0,0,255);
					}
				}
			}

			if(!util::c_isspace(text_[n][m]) && util::c_isprint(text_[n][m])) {
				const CharArea& area = get_char_area(font_size_, text_[n][m]);

				const int x1 = xpos + c*char_width_;
				const int y1 = ypos + r*char_height_;
				const int x2 = x1 + char_width_;
				const int y2 = y1 + char_height_;

				graphics::blit_queue& q = chars[col.rgba()];

				q.repeat_last();
				q.add(x1, y1, area.x1, area.y1);
				q.repeat_last();
				q.add(x2, y1, area.x2, area.y1);
				q.add(x1, y2, area.x1, area.y2);
				q.add(x2, y2, area.x2, area.y2);
			}

			if(cursor_.row == n && cursor_.col == m &&
			   (SDL_GetTicks()%500 < 350 || !has_focus_)) {
				RectDraw rect_draw = { rect(xpos + c*char_width_+1, ypos + r*char_height_, 1, char_height_), graphics::color(255,255,255,255) };
				rects.push_back(rect_draw);
			}

			if(text_[n][m] == '\t') {
				c += TabAdjust;
			}
		}

		if(has_focus_ && cursor_.row == n && cursor_.col >= text_[n].size() && SDL_GetTicks()%500 < 350) {
			RectDraw rect_draw = { rect(xpos + c*char_width_+1, ypos + r*char_height_, 1, char_height_), graphics::color(255,255,255,255) };
			rects.push_back(rect_draw);
		}
	}

	const int begin_draw = SDL_GetTicks();

	foreach(const RectDraw& r, rects) {
		graphics::draw_rect(r.area, r.col);
	}

	for(std::map<uint32_t, graphics::blit_queue>::iterator i = chars.begin(); i != chars.end(); ++i) {
		graphics::color(i->first).set_as_current_color();
		i->second.set_texture(char_textures[font_size_]->get_id());
		i->second.do_blit();
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
	if(!claimed) {
		claimed = clipboard_handle_event(event);
	}

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

void text_editor_widget::set_focus(bool value)
{
	if(has_focus_ != value && on_change_focus_) {
		on_change_focus_(value);
	}
	has_focus_ = value;

	if(value) {
		SDL_EnableUNICODE(1);
		SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
	}

	if(nrows_ == 1 && value) {
		cursor_ = Loc(0, text_.front().size());
		select_ = Loc(0, 0);
		on_move_cursor();
	}
}

void text_editor_widget::set_cursor(int row, int col)
{
	if(row < 0) {
		row = 0;
	}

	if(col < 0) {
		col = 0;
	}

	if(row >= text_.size()) {
		row = text_.size() - 1;
	}

	if(col > text_[row].size()) {
		col = text_[row].size();
	}

	select_ = cursor_ = Loc(row, col);

	on_move_cursor();
}

int text_editor_widget::row_col_to_text_pos(int row, int col) const
{
	if(col > text_[row].size()) {
		col = text_[row].size();
	}

	int result = 0;
	for(int n = 0; n != row; ++n) {
		result += text_[n].size() + 1;
	}

	return result + col;
}

std::pair<int,int> text_editor_widget::text_pos_to_row_col(int pos) const
{
	int nrow = 0;
	while(pos > text_[nrow].size()+1) {
		pos -= text_[nrow].size()+1;
		++nrow;
	}

	return std::pair<int,int>(nrow, pos);
}

void text_editor_widget::set_highlight_lines(int begin, int end)
{
	begin_highlight_line_ = begin;
	end_highlight_line_ = end;
}

void text_editor_widget::clear_highlight_lines()
{
	set_highlight_lines(-1, -1);
}

bool text_editor_widget::handle_mouse_button_down(const SDL_MouseButtonEvent& event)
{
	record_op();
	if(event.x >= x() && event.x < x() + width() && event.y >= y() && event.y < y() + height()) {
		if(event.button == SDL_BUTTON_WHEELUP) {
			if(cursor_.row > 2) {
				cursor_.row -= 3;
				scroll_pos_ -= 3;
				if( scroll_pos_ < 0 ){ 
					scroll_pos_ = 0; 
				}
				cursor_.col = find_equivalent_col(cursor_.col, cursor_.row+3, cursor_.row);
				on_move_cursor();
			}
			return true;
		} else if(event.button == SDL_BUTTON_WHEELDOWN) {
			if(text_.size() > 2 && cursor_.row < text_.size()-3) {
				cursor_.row += 3;
				scroll_pos_ += 3;
				if( scroll_pos_ > text_.size() ){ 
					scroll_pos_ = text_.size(); 
				}
				cursor_.col = find_equivalent_col(cursor_.col, cursor_.row-3, cursor_.row);
				on_move_cursor();
			}
			return true;
		}

		set_focus(true);
		std::pair<int, int> pos = mouse_position_to_row_col(event.x, event.y);
		if(pos.first != -1) {
			cursor_.row = pos.first;
			cursor_.col = pos.second;
			on_move_cursor();
		}

		if(last_click_at_ != -1 && SDL_GetTicks() - last_click_at_ < 500) {
			++consecutive_clicks_;

			const int nclicks = consecutive_clicks_%3;

			if(nclicks == 1) {
				select_ = cursor_;
				select_token(text_[cursor_.row], select_.row, cursor_.row, select_.col, cursor_.col);
			} else if(nclicks == 2) {
				select_ = Loc(cursor_.row, 0);
				cursor_.col = text_[cursor_.row].size();
			}

			if(select_ != cursor_) {
				//a mouse-based copy for X-style copy/paste
				handle_copy(true);
			}
		} else {
			consecutive_clicks_ = 0;

			if(event.button == SDL_BUTTON_MIDDLE && clipboard_has_mouse_area()) {
				std::string txt = copy_from_clipboard(true);
				handle_paste(txt);
			}
		}

		last_click_at_ = SDL_GetTicks();

		is_dragging_ = true;
		return true;
	}

	if(has_focus_ != false && on_change_focus_) {
		on_change_focus_(false);
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
			cursor_.row = pos.first;
			cursor_.col = pos.second;
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
		handle_copy();

		if(event.keysym.sym == SDLK_x) {
			save_undo_state();
			delete_selection();
			on_change();
		}

		return true;
	} else if(event.keysym.sym == SDLK_v && (event.keysym.mod&KMOD_CTRL)) {
		handle_paste(copy_from_clipboard(false));

		return true;
	}

	if(event.keysym.mod&KMOD_CTRL) {
		if(event.keysym.sym == SDLK_BACKSPACE) {
			if(select_ == cursor_) {
				//We delete the current word behind us. 
				truncate_col_position();

				if(cursor_.col > 0) {
					save_undo_state();
				}

				const std::string& line = text_[select_.row];
				int col = select_.col;
				while(col > 0 && !(util::c_isalnum(line[col-1]) || line[col-1] == '_')) {
					--col;
				}

				while(col > 0 && (util::c_isalnum(line[col-1]) || line[col-1] == '_')) {
					--col;
				}

				select_.col = col;
				delete_selection();
				record_op();
				return true;
			}
		} else if(event.keysym.sym == SDLK_DELETE) {
			if(select_ == cursor_) {
				//We delete until end of line.
				truncate_col_position();

				if(cursor_.col < text_[select_.row].size()) {
					save_undo_state();
				}

				select_ = Loc(select_.row, text_[select_.row].size());
				delete_selection();
				record_op();
				return true;
			}
		} else { 
			record_op();
			return false;
		}
	}

	if(event.keysym.sym == SDLK_ESCAPE && on_escape_) {
		on_escape_();
		return true;
	}

	switch(event.keysym.sym) {
	case SDLK_LEFT:
		record_op();

		if(cursor_ != select_ && !(SDL_GetModState()&KMOD_SHIFT)) {
			//pressing left without shift while we have a selection moves us to the beginning of the selection
			if(cursor_ < select_) {
				select_ = cursor_;
			} else {
				cursor_ = select_;
			}
		} else {

			if(cursor_.col > text_[cursor_.row].size()) {
				cursor_.col = text_[cursor_.row].size();
			}

			--cursor_.col;
			if(cursor_.col < 0) {
				if(cursor_.row == 0) {
					cursor_.col = 0;
				} else {
					--cursor_.row;
					cursor_.col = text_[cursor_.row].size();
				}
			}
		}

		on_move_cursor();
		break;
	case SDLK_RIGHT:
		record_op();

		if(cursor_ != select_ && !(SDL_GetModState()&KMOD_SHIFT)) {
			//pressing right without shift while we have a selection moves us to the end of the selection
			if(cursor_ < select_) {
				cursor_ = select_;
			} else {
				select_ = cursor_;
			}
		} else {
			++cursor_.col;
			if(cursor_.col > text_[cursor_.row].size()) {
				if(cursor_.row == text_.size()-1) {
					--cursor_.col;
				} else if(cursor_.row < text_.size()-1) {
					++cursor_.row;
					cursor_.col = 0;
				} else {
					--cursor_.col;
				}
			}
		}
		on_move_cursor();
		break;
	case SDLK_UP:
		record_op();
		if(cursor_.row > 0) {
			--cursor_.row;
			cursor_.col = find_equivalent_col(cursor_.col, cursor_.row+1, cursor_.row);
		}
		on_move_cursor();

		break;
	case SDLK_DOWN:
		record_op();
		if(cursor_.row < text_.size()-1) {
			++cursor_.row;
			cursor_.col = find_equivalent_col(cursor_.col, cursor_.row-1, cursor_.row);
		}
		on_move_cursor();

		break;
	case SDLK_PAGEUP: {
		record_op();
		on_page_up();
		bool move_cursor = false;
		while(cursor_.row > scroll_pos_ && char_position_on_screen(cursor_.row, cursor_.col).first == -1) {
			--cursor_.row;
			cursor_.col = find_equivalent_col(cursor_.col, cursor_.row+1, cursor_.row);
			move_cursor = true;
		}

		if(move_cursor) {
			on_move_cursor();
		}

		if(!(SDL_GetModState()&KMOD_SHIFT)) {
			select_ = cursor_;
		}
		break;
	}

	case SDLK_PAGEDOWN: {
		record_op();
		on_page_down();
		bool move_cursor = false;
		while(cursor_.row < scroll_pos_ && char_position_on_screen(cursor_.row, cursor_.col).first == -1) {
			++cursor_.row;
			cursor_.col = find_equivalent_col(cursor_.col, cursor_.row-1, cursor_.row);
			move_cursor = true;
		}

		if(move_cursor) {
			on_move_cursor();
		}

		if(!(SDL_GetModState()&KMOD_SHIFT)) {
			select_ = cursor_;
		}
		break;
	}
	case SDLK_HOME:
		record_op();
		cursor_.col = 0;
		on_move_cursor();
		break;
	case SDLK_END:
		record_op();
		cursor_.col = text_[cursor_.row].size();
		on_move_cursor();
		break;
	case SDLK_DELETE:
	case SDLK_BACKSPACE:
		if(record_op("delete")) {
			save_undo_state();
		}
		if(cursor_ == select_) {

			if(event.keysym.sym == SDLK_BACKSPACE) {
				//backspace is like delete but we move to the left first.
				if(cursor_.col > text_[cursor_.row].size()) {
					cursor_.col = text_[cursor_.row].size();
				}

				if(cursor_.row == 0 && cursor_.col == 0) {
					break;
				}

				--cursor_.col;
				if(cursor_.col < 0) {
					--cursor_.row;
					cursor_.col = text_[cursor_.row].size();
				}

				on_move_cursor();
			}

			if(cursor_.col >= text_[cursor_.row].size()) {
				if(text_.size() > cursor_.row+1) {
					cursor_.col = text_[cursor_.row].size();
					text_[cursor_.row] += text_[cursor_.row+1];
					text_.erase(text_.begin() + cursor_.row + 1);
				}
			} else {
				text_[cursor_.row].erase(text_[cursor_.row].begin() + cursor_.col);
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
			if(on_enter_) {
				on_enter_();
			}
			break;
		}

		if(on_begin_enter_) {
			if(!on_begin_enter_()) {
				break;
			}
		}

		delete_selection();
		truncate_col_position();
		
		std::string new_line(text_[cursor_.row].begin() + cursor_.col, text_[cursor_.row].end());
		text_[cursor_.row].erase(text_[cursor_.row].begin() + cursor_.col, text_[cursor_.row].end());

		std::string::iterator indent = text_[cursor_.row].begin();
		while(indent != text_[cursor_.row].end() && strchr(" \t", *indent)) {
			++indent;
		}

		new_line.insert(new_line.begin(), text_[cursor_.row].begin(), indent);

		cursor_.col = indent - text_[cursor_.row].begin();

		text_.insert(text_.begin() + cursor_.row + 1, new_line);
		++cursor_.row;
		select_ = cursor_;

		refresh_scrollbar();
		on_change();
		on_move_cursor();

		if(on_enter_) {
			on_enter_();
		}
		
		break;
	}
	case SDLK_TAB: {
		if(on_tab_) {
			on_tab_();
		}

		if(nrows_ == 1) {
			break;
		}
	}
	default: {
		const char c = event.keysym.unicode;
		if(util::c_isprint(c) || c == '\t') {
			if(record_op("chars")) {
				save_undo_state();
			}
			delete_selection();
			if(cursor_.col > text_[cursor_.row].size()) {
				cursor_.col = text_[cursor_.row].size();
			}
			text_[cursor_.row].insert(text_[cursor_.row].begin() + cursor_.col, c);
			++cursor_.col;
			select_ = cursor_;
			refresh_scrollbar();
			on_change();
			return true;
		}
		return false;
	}
	}

	return true;
}

void text_editor_widget::handle_paste(std::string txt)
{
	record_op();
	save_undo_state();
	delete_selection();

	txt.erase(std::remove(txt.begin(), txt.end(), '\r'), txt.end());
	std::vector<std::string> lines = util::split(txt, '\n', 0 /*don't remove empties or strip spaces*/);

	truncate_col_position();

	if(lines.size() == 1) {
		text_[cursor_.row].insert(text_[cursor_.row].begin() + cursor_.col, lines.front().begin(), lines.front().end());
		cursor_.col += lines.front().size();
		refresh_scrollbar();
		select_ = cursor_;
	} else if(lines.size() >= 2) {
		text_.insert(text_.begin() + cursor_.row + 1, lines.back() + std::string(text_[cursor_.row].begin() + cursor_.col, text_[cursor_.row].end()));
		text_[cursor_.row] = std::string(text_[cursor_.row].begin(), text_[cursor_.row].begin() + cursor_.col) + lines.front();
		text_.insert(text_.begin() + cursor_.row + 1, lines.begin()+1, lines.end()-1);
		cursor_ = select_ = Loc(cursor_.row + lines.size() - 1, lines.back().size());
	}

	on_change();
}

void text_editor_widget::handle_copy(bool mouse_based)
{
	if(mouse_based && !clipboard_has_mouse_area()) {
		return;
	}

	Loc begin = cursor_;
	Loc end = select_;

	if(begin.col > text_[begin.row].size()) {
		begin.col = text_[begin.row].size();
	}

	if(end.col > text_[end.row].size()) {
		end.col = text_[end.row].size();
	}

	if(end < begin) {
		std::swap(begin, end);
	}


	std::string str;
	if(begin.row == end.row) {
		str = std::string(text_[begin.row].begin() + begin.col, text_[begin.row].begin() + end.col);
	} else {
		str = std::string(text_[begin.row].begin() + begin.col, text_[begin.row].end());
		while(++begin.row < end.row) {
			str += "\n" + text_[begin.row];
		}

		str += "\n" + std::string(text_[end.row].begin(), text_[end.row].begin() + end.col);
	}

	copy_to_clipboard(str, mouse_based);
}

void text_editor_widget::delete_selection()
{
	if(cursor_.col == select_.col && cursor_.row == select_.row) {
		return;
	}

	if(cursor_.col > text_[cursor_.row].size()) {
		cursor_.col = text_[cursor_.row].size();
	}

	if(select_.col > text_[select_.row].size()) {
		select_.col = text_[select_.row].size();
	}

	if(select_ < cursor_) {
		std::swap(cursor_, select_);
	}

	std::string& cursor_line = text_[cursor_.row];
	std::string& select_line = text_[select_.row];
	if(cursor_.row == select_.row) {
		cursor_line.erase(cursor_line.begin() + cursor_.col, cursor_line.begin() + select_.col);
	} else {
		cursor_line = std::string(cursor_line.begin(), cursor_line.begin() + cursor_.col) + std::string(select_line.begin() + select_.col, select_line.end());

		text_.erase(text_.begin() + cursor_.row + 1, text_.begin() + select_.row + 1);
	}

	select_ = cursor_;
}

graphics::color text_editor_widget::get_character_color(int row, int col) const
{
	return text_color_;
}

std::pair<int, int> text_editor_widget::mouse_position_to_row_col(int xpos, int ypos) const
{
	const int xloc = x() + BorderSize;
	const int yloc = y() + BorderSize;

	int r = 0;
	for(int n = scroll_pos_; n < text_.size() && r < nrows_; ++n, ++r) {
		int c = 0;
		bool matches_row = ypos >= yloc + r*char_height_ && ypos < yloc + (r+1)*char_height_;
		for(size_t m = 0; m < text_[n].size(); ++m, ++c) {
			if(c >= ncols_) {
				if(matches_row) {
					break;
				}
				++r;
				c -= ncols_;
				matches_row = ypos >= yloc + r*char_height_ && ypos < yloc + (r+1)*char_height_;
				if(r == nrows_) {
					break;
				}
			}

			const int char_size = text_[n][m] == '\t' ? TabWidth : 1;

			if(matches_row && xpos >= xloc + c*char_width_ && xpos < xloc + (c+char_size)*char_width_) {
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
	for(size_t n = scroll_pos_; n < text_.size() && r < nrows_; ++n, ++r) {
		int c = 0;
		size_t m;
		for(m = 0; m < text_[n].size(); ++m, ++c) {
			if(c >= ncols_) {
				++r;
				c -= ncols_;
				if(r == nrows_) {
					break;
				}
			}

			if(row == n && col == m) {
				return std::pair<int, int>(BorderSize + r*char_height_, BorderSize + c*char_width_);
			}

			if(text_[n][m] == '\t') {
				c += TabAdjust;
				continue;
			}
		}

		if(row == n && m == text_[n].size()) {
			return std::pair<int, int>(BorderSize + r*char_height_, BorderSize + c*char_width_);
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
	if(cursor_.row < scroll_pos_) {
		scroll_pos_ = cursor_.row;
	} else {
		while(scroll_pos_ < cursor_.row && char_position_on_screen(cursor_.row, cursor_.col).first == -1) {
			++scroll_pos_;
		}
	}

	if(start_pos != scroll_pos_) {
		refresh_scrollbar();
	}

	if(!auto_shift && !(SDL_GetModState()&KMOD_SHIFT)) {
		select_ = cursor_;
	}

	scrollable_widget::set_yscroll(scroll_pos_*char_height_);

	if(select_ != cursor_) {
		//a mouse-based copy for X-style copy/paste
		handle_copy(true);
	}

	if(on_move_cursor_) {
		on_move_cursor_();
	}
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

	set_yscroll(scroll_pos_*char_height_);

	update_scrollbar();
}

void text_editor_widget::select_token(const std::string& row, int& begin_row, int& end_row, int& begin_col, int& end_col)
{
	if(util::c_isdigit(row[begin_col]) || row[begin_col] == '.' && begin_col+1 < row.size() && util::c_isdigit(row[begin_col+1])) {
		while(begin_col >= 0 && (util::c_isdigit(row[begin_col]) || row[begin_col] == '.')) {
			--begin_col;
		}

		if(begin_col < 0 || row[begin_col] != '-') {
			++begin_col;
		}

		while(end_col < row.size() && (util::c_isdigit(row[end_col]) || row[end_col] == '.')) {
			++end_col;
		}
	} else if(util::c_isalnum(row[begin_col]) || row[begin_col] == '_') {
		while(begin_col >= 0 && (util::c_isalnum(row[begin_col]) || row[begin_col] == '_')) {
			--begin_col;
		}

		++begin_col;

		while(end_col < row.size() && (util::c_isalnum(row[end_col]) || row[end_col] == '_')) {
			++end_col;
		}
	} else if(end_col < row.size()) {
		++end_col;
	}
}

text_editor_widget_ptr text_editor_widget::clone() const
{
	text_editor_widget_ptr result = new text_editor_widget(*this);
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
	undo_.push_back(text_editor_widget_ptr(clone()));
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

	std::vector<text_editor_widget_ptr> redo_state = redo_;
	save_undo_state();
	redo_state.push_back(undo_.back());
	undo_.pop_back();

	//Save the state before restoring it so it doesn't get cleaned up
	//while we're in the middle of the restore call.
	text_editor_widget_ptr state = undo_.back();
	restore(state.get());

	redo_ = redo_state;

	on_change();
}

void text_editor_widget::redo()
{
	if(redo_.empty()) {
		return;
	}

	std::vector<text_editor_widget_ptr> redo_state = redo_;
	redo_state.pop_back();

	//Save the state before restoring it so it doesn't get cleaned up
	//while we're in the middle of the restore call.
	text_editor_widget_ptr state = redo_.back();
	restore(state.get());

	redo_ = redo_state;

	on_change();
}

void text_editor_widget::truncate_col_position()
{
	if(cursor_.col > text_[cursor_.row].size()) {
		cursor_.col = text_[cursor_.row].size();
	}

	if(select_.col > text_[select_.row].size()) {
		select_.col = text_[select_.row].size();
	}
}

void text_editor_widget::set_search(const std::string& term)
{
	search_ = term;
	calculate_search_matches();
	if(search_matches_.empty()) {
		return;
	}

	std::vector<std::pair<Loc, Loc> >::const_iterator search_itor =
	   std::lower_bound(search_matches_.begin(), search_matches_.end(),
	                    std::pair<Loc,Loc>(cursor_, cursor_));
	if(search_itor == search_matches_.end()) {
		search_itor = search_matches_.begin();
	}

	select_ = cursor_ = search_itor->first;

	on_move_cursor();
}

void text_editor_widget::next_search_match()
{
	if(search_matches_.empty()) {
		return;
	}

	cursor_.col++;
	select_ = cursor_;
	set_search(search_);
}

void text_editor_widget::calculate_search_matches()
{
	search_matches_.clear();
	if(search_.empty()) {
		return;
	}

	try {
		boost::regex re(search_, boost::regex::perl|boost::regex::icase);
		for(int n = 0; n != text_.size(); ++n) {
			boost::cmatch match;
			const char* ptr = text_[n].c_str();
			while(boost::regex_search(ptr, match, re)) {
				const int base = ptr - text_[n].c_str();
				const Loc begin(n, base + match.position());
				const Loc end(n, base + match.position() + match.length());
				search_matches_.push_back(std::pair<Loc,Loc>(begin,end));
	
				const int advance = match.position() + match.length();
				if(advance == 0) {
					break;
				}
	
				ptr += advance;
			}
		}
	} catch(boost::regex_error&) {
	}
}

void text_editor_widget::replace(const std::string& replace_with)
{
	record_op();
	save_undo_state();
	
	//we have to get the end itor here because some compilers don't
	//support comparing a const and non-const reverse iterator
	const std::vector<std::pair<Loc, Loc> >::const_reverse_iterator end_itor = search_matches_.rend();
	for(std::vector<std::pair<Loc, Loc> >::const_reverse_iterator i = search_matches_.rbegin(); i != end_itor; ++i) {
		const Loc& begin = i->first;
		const Loc& end = i->second;
		if(begin.row != end.row) {
			continue;
		}

		text_[begin.row].erase(text_[begin.row].begin() + begin.col, text_[begin.row].begin() + end.col);
		text_[begin.row].insert(text_[begin.row].begin() + begin.col, replace_with.begin(), replace_with.end());
	}

	on_change();
}
	
void text_editor_widget::on_change()
{
	if(on_change_) {
		on_change_();
	}

	calculate_search_matches();
}

void text_editor_widget::set_value(const std::string& key, const variant& v)
{
	if(key == "text") {
		set_text(v.as_string());
	} else if(key == "begin_enter") {
		begin_enter_return_ = v.as_bool();
	}
	scrollable_widget::set_value(key, v);
}

variant text_editor_widget::get_value(const std::string& key) const
{
	if(key == "text" ) {
		return variant(text());
	}
	return scrollable_widget::get_value(key);
}

void text_editor_widget::change_delegate()
{
	using namespace game_logic;
	if(get_environment()) {
		map_formula_callable_ptr callable = map_formula_callable_ptr(new map_formula_callable(get_environment()));
		callable->add("text", variant(text()));
		variant value = ffl_on_change_->execute(*callable);
		get_environment()->execute_command(value);
	} else {
		std::cerr << "text_editor_widget::change_delegate() called without environment!" << std::endl;
	}
}

void text_editor_widget::move_cursor_delegate()
{
	if(get_environment()) {
		variant value = ffl_on_move_cursor_->execute(*get_environment());
		get_environment()->execute_command(value);
	} else {
		std::cerr << "text_editor_widget::move_cursor_delegate() called without environment!" << std::endl;
	}
}

void text_editor_widget::enter_delegate()
{
	using namespace game_logic;
	if(get_environment()) {
		map_formula_callable_ptr callable = map_formula_callable_ptr(new map_formula_callable(get_environment()));
		callable->add("text", variant(text()));
		variant value = ffl_on_enter_->execute(*callable);
		get_environment()->execute_command(value);
	} else {
		std::cerr << "text_editor_widget::enter_delegate() called without environment!" << std::endl;
	}
}

void text_editor_widget::escape_delegate()
{
	using namespace game_logic;
	if(get_environment()) {
		map_formula_callable_ptr callable = map_formula_callable_ptr(new map_formula_callable(get_environment()));
		callable->add("text", variant(text()));
		variant value = ffl_on_escape_->execute(*callable);
		get_environment()->execute_command(value);
	} else {
		std::cerr << "text_editor_widget::escape_delegate() called without environment!" << std::endl;
	}
}

void text_editor_widget::tab_delegate()
{
	using namespace game_logic;
	if(get_environment()) {
		map_formula_callable_ptr callable = map_formula_callable_ptr(new map_formula_callable(get_environment()));
		callable->add("text", variant(text()));
		variant value = ffl_on_tab_->execute(*callable);
		get_environment()->execute_command(value);
	} else {
		std::cerr << "text_editor_widget::tab_delegate() called without environment!" << std::endl;
	}
}

bool text_editor_widget::begin_enter_delegate()
{
	if(get_environment()) {
		variant value = ffl_on_begin_enter_->execute(*get_environment());
		get_environment()->execute_command(value);
	} else {
		std::cerr << "text_editor_widget::begin_enter_delegate() called without environment!" << std::endl;
	}
	// XXX Need some way of doing the return value here.
	return begin_enter_return_;
}

void text_editor_widget::change_focus_delgate(bool new_focus_value)
{
	using namespace game_logic;
	if(get_environment()) {
		map_formula_callable_ptr callable = map_formula_callable_ptr(new map_formula_callable(get_environment()));
		callable->add("focus", variant::from_bool(new_focus_value));
		callable->add("text", variant(text()));
		variant value = ffl_on_change_focus_->execute(*callable);
		get_environment()->execute_command(value);
	} else {
		std::cerr << "text_editor_widget::tab_delegate() called without environment!" << std::endl;
	}
}

}

#include "code_editor_widget.hpp"
#include "dialog.hpp"
#include "filesystem.hpp"

namespace {
void on_change_search(const gui::text_editor_widget_ptr search_entry, gui::text_editor_widget_ptr editor)
{
	editor->set_search(search_entry->text());
}
}

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

	text_editor_widget_ptr entry = new text_editor_widget(120);

	text_editor_widget_ptr editor = new code_editor_widget(600, 400);
	editor->set_text(contents);

	entry->set_on_change_handler(boost::bind(on_change_search, entry, editor));
	entry->set_on_enter_handler(boost::bind(&text_editor_widget::next_search_match, editor));

	dialog d(0, 0, graphics::screen_width(), graphics::screen_height());
	d.add_widget(widget_ptr(entry), 10, 10);
	d.add_widget(widget_ptr(editor), 10, 30);
	d.show_modal();
}

UNIT_TEST(test_regex)
{
	std::string searching = "abcdefg";
	boost::regex re("cde");
	boost::cmatch matches;
	const char* ptr = searching.c_str();
	if(boost::regex_search(ptr, matches, re)) {
		CHECK_EQ(matches.size(), 1);
		CHECK_EQ(matches.position(), 2);
		CHECK_EQ(matches.length(), 3);
	}
}
