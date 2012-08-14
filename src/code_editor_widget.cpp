#include "asserts.hpp"
#include "code_editor_widget.hpp"
#include "decimal.hpp"
#include "formula_tokenizer.hpp"
#include "json_parser.hpp"
#include "string_utils.hpp"
#include "utility_query.hpp"

#include <boost/bind.hpp>
#include <boost/regex.hpp>

#include <stack>

namespace gui
{

code_editor_widget::code_editor_widget(int width, int height)
  : text_editor_widget(width, height),
  row_slider_(0), begin_col_slider_(0), end_col_slider_(0),
  slider_decimal_(false), slider_magnitude_(0), is_formula_(false)
{
	set_environment();
}

code_editor_widget::code_editor_widget(const variant& v, game_logic::formula_callable* e) 
	: text_editor_widget(v,e), row_slider_(0), begin_col_slider_(0), 
	end_col_slider_(0),	slider_decimal_(false), slider_magnitude_(0), is_formula_(false)
{
}

void code_editor_widget::on_move_cursor(bool auto_shift)
{
	text_editor_widget::on_move_cursor(auto_shift);

	ObjectInfo info = get_current_object();
}

void code_editor_widget::on_change()
{
	generate_tokens();

	bracket_match_.clear();
	colors_.clear();
	colors_.resize(colors_.size()+1);
	const std::string s = (is_formula_ ? "\"" : "") + text() + (is_formula_ ? "\"" : "");
	std::string::const_iterator i = s.begin();
	while(i != s.end()) {
		if(*i == '"') {
			std::vector<std::vector<std::pair<int, int> > > opening_brackets;

			if(!is_formula_) {
				colors_.back().push_back(graphics::color(196, 196, 196));
			}
			++i;
			std::string::const_iterator end = i;
			while(end != s.end() && *end != '"') {
				if(*end == '\\') {
					++end;
				}

				++end;
			}

			if(end != s.end()) {
				while(i != end) {
					std::string::const_iterator begin = i;
					try {
						formula_tokenizer::token t = formula_tokenizer::get_token(i, end);

						bool error_color = false;
						switch(t.type) {
						case formula_tokenizer::TOKEN_LPARENS:
						case formula_tokenizer::TOKEN_LSQUARE:
						case formula_tokenizer::TOKEN_LBRACKET:
							opening_brackets.resize(opening_brackets.size()+1);
							opening_brackets.back().push_back(std::pair<int,int>(colors_.size()-1, colors_.back().size()));
							break;
						case formula_tokenizer::TOKEN_RPARENS:
						case formula_tokenizer::TOKEN_RSQUARE:
						case formula_tokenizer::TOKEN_RBRACKET:
							if(opening_brackets.empty()) {
								error_color = true;
							} else {
								opening_brackets.back().push_back(std::pair<int,int>(colors_.size()-1, colors_.back().size()));
								std::pair<int,int> key(colors_.size()-1, colors_.back().size());
								for(int n = 0; n != opening_brackets.back().size(); ++n) {
									bracket_match_[opening_brackets.back()[n]] = opening_brackets.back();
								}
								opening_brackets.pop_back();
							}
							break;
						case formula_tokenizer::TOKEN_COMMA:
							if(opening_brackets.empty() == false) {
								opening_brackets.back().push_back(std::pair<int,int>(colors_.size()-1, colors_.back().size()));
							}
							break;
						}

						if(t.type == formula_tokenizer::TOKEN_OPERATOR && util::c_isalpha(*t.begin)) {
							t.type = formula_tokenizer::TOKEN_KEYWORD;

						}
						
						while(begin != i) {
							if(*begin == '\n') {
								colors_.resize(colors_.size()+1);
							} else {
static const graphics::color TokenColors[] = {
	graphics::color(128, 128, 255), //operator
	graphics::color(64, 255, 64), //string literal
	graphics::color(196, 196, 196), //const identifier
	graphics::color(255, 255, 255), //identifier
	graphics::color(255, 196, 196), //integer
	graphics::color(255, 196, 196), //decimal
	graphics::color(128, 128, 255), //lparens
	graphics::color(128, 128, 255), //rparens
	graphics::color(128, 128, 255), //lsquare
	graphics::color(128, 128, 255), //rsquare
	graphics::color(128, 128, 255), //lbracket
	graphics::color(128, 128, 255), //rbracket
	graphics::color(128, 128, 255), //comma
	graphics::color(128, 128, 255), //semi
	graphics::color(128, 128, 255), //colon
	graphics::color(255, 255, 255), //whitespace
	graphics::color(64, 255, 64), //keyword
	graphics::color(64, 255, 64), //comment
	graphics::color(255, 255, 255), //pointer
};
								graphics::color col(255, 255, 255);
								if(t.type >= 0 && t.type < sizeof(TokenColors)/sizeof(TokenColors[0])) {
									col = TokenColors[t.type];
								}

								if(error_color) {
									col = graphics::color(255, 0, 0);
								}

								colors_.back().push_back(col);
							}
							++begin;
						}

					} catch(formula_tokenizer::token_error&) {
						i = begin;
						break;
					}
				}

				for(int n = 0; n != opening_brackets.size(); ++n) {
					//any remaining brackets that weren't matched can be marked as errors.
					colors_[opening_brackets[n].front().first][opening_brackets[n].front().second] = graphics::color(255, 0, 0);
				}

				if(i == end) {
					colors_.back().push_back(graphics::color(196, 196, 196));
					i = end + 1;
				}
			}


		} else if(*i == '\n') {
			colors_.resize(colors_.size()+1);
			++i;
		} else {
			colors_.back().push_back(graphics::color(255, 255, 255));
			++i;
		}
	}

	text_editor_widget::on_change();
}

graphics::color code_editor_widget::get_character_color(int row, int col) const
{
	std::map<std::pair<int, int>, std::vector<std::pair<int, int> > >::const_iterator itor = bracket_match_.find(std::pair<int,int>(row,col));
	if(itor != bracket_match_.end()) {
		for(int n = 0; n != itor->second.size(); ++n) {
			const int match_row = itor->second[n].first;
			const int match_col = itor->second[n].second;
			if(cursor_row() == match_row) {
				if(cursor_col() == match_col+1 || colors_[match_row].size() == match_col+1 && cursor_col() > match_col+1) {
					return graphics::color(255, 0, 0);
				}
			}
		}
	}

	ASSERT_LOG(row >= 0 && row < colors_.size(), "Invalid row: " << row << " /" << colors_.size());
	ASSERT_LOG(col >= 0 && col < colors_[row].size(), "Invalid col: " << col << " /" << colors_[row].size());
	return colors_[row][col];
}

void code_editor_widget::select_token(const std::string& row, int& begin_row, int& end_row, int& begin_col, int& end_col)
{
	std::pair<int,int> key(begin_row, begin_col);
	if(bracket_match_.count(key)) {
		begin_row = bracket_match_.find(key)->second.front().first;
		begin_col = bracket_match_.find(key)->second.front().second;
		end_row = bracket_match_.find(key)->second.back().first;
		end_col = bracket_match_.find(key)->second.back().second+1;
		return;
	}

	text_editor_widget::select_token(row, begin_row, end_row, begin_col, end_col);

	std::string token(row.begin() + begin_col, row.begin() + end_col);
	
	boost::regex numeric_regex("-?\\d+(\\.\\d+)?", boost::regex::perl);
	std::cerr << "token: (" << token << ")\n";
	if(boost::regex_match(token.c_str(), numeric_regex)) {

		const decimal current_value(decimal::from_string(token));
		if(current_value <= 10000000 && current_value >= -10000000) {
			slider_.reset(new slider(200, boost::bind(&code_editor_widget::on_slider_move, this, _1)));
			slider_decimal_ = std::count(token.begin(), token.end(), '.') ? true : false;
			slider_magnitude_ = (abs(current_value.as_int())+1) * 5;
	
			const decimal slider_value = (current_value - decimal::from_int(-slider_magnitude_)) / decimal::from_int(slider_magnitude_*2);
			slider_->set_position(slider_value.as_float());

			std::pair<int,int> pos = char_position_on_screen(begin_row, (begin_col+end_col)/2);

			row_slider_ = begin_row;
			begin_col_slider_ = begin_col;
			end_col_slider_ = end_col;

			int x = pos.second - slider_->width()/2;
			int y = pos.first + 20 - slider_->height();
			if(x < 10) {
				x = 10;
			}

			if(x > width() - slider_->width()) {
				x = width() - slider_->width();
			}

			if(y < 20) {
				y += 60;
			}

			if(y > height() - slider_->height()) {
				y = height() - slider_->height();
			}
	
			slider_->set_loc(x, y);
		}
	}
}

void code_editor_widget::on_slider_move(double value)
{
	if(record_op("slider")) {
		save_undo_state();
	}

	std::ostringstream s;
	value = (value - 0.5)*2.0; // normalize to [-1.0,1.0] range.

	const decimal new_value(value*slider_magnitude_);
	if(slider_decimal_) {
		s << new_value;
	} else {
		s << new_value.as_int();
	}

	std::string new_string = s.str();

	ASSERT_LOG(row_slider_ >= 0 && row_slider_ < get_data().size(), "Illegal row value for slider: " << row_slider_ << " / " << get_data().size());
	std::string row = get_data()[row_slider_];

	row.erase(row.begin() + begin_col_slider_, row.begin() + end_col_slider_);
	row.insert(row.begin() + begin_col_slider_, new_string.begin(), new_string.end());

	const int old_end = end_col_slider_;
	end_col_slider_ = begin_col_slider_ + new_string.size();

	if(cursor_row() == row_slider_ && cursor_col() == old_end) {
		set_cursor(cursor_row(), end_col_slider_);
	}

	set_row_contents(row_slider_, row);
}

void code_editor_widget::handle_draw() const
{
	text_editor_widget::handle_draw();

	if(slider_) {
		slider_->draw();
	}
}

bool code_editor_widget::handle_event(const SDL_Event& event, bool claimed)
{
	if(slider_) {
		if(slider_->process_event(event, claimed)) {
			return true;
		}
	}

	if(event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_KEYDOWN) {
		slider_.reset();
	}

	return text_editor_widget::handle_event(event, claimed) || claimed;
}

void code_editor_widget::generate_tokens()
{
	current_text_ = text();

	try {
		current_obj_ = json::parse(current_text_);
	} catch(...) {
	}

	tokens_.clear();
	const char* begin = current_text_.c_str();
	const char* end = begin + current_text_.size();

	try {
		json::Token token = json::get_token(begin, end);
		while(token.type != json::Token::NUM_TYPES) {
			tokens_.push_back(token);
			token = json::get_token(begin, end);
		}
	} catch(json::TokenizerError& e) {
		std::cerr << "Tokenizer error: " << e.msg << "\n";
	}
}

namespace {
variant get_map_editing(int row, int col, variant item)
{
	if(!item.get_debug_info()) {
		return variant();
	}

	const int begin_row = item.get_debug_info()->line;
	const int begin_col = item.get_debug_info()->column;
	const int end_row = item.get_debug_info()->end_line;
	const int end_col = item.get_debug_info()->end_column;

	typedef text_editor_widget::Loc Loc;

	if(Loc(row,col) < Loc(begin_row,begin_col) ||
	   Loc(row,col) > Loc(end_row,end_col)) {
		return variant();
	}

	if(item.is_list()) {
		foreach(variant v, item.as_list()) {
			variant result = get_map_editing(row, col, v);
			if(result.is_null() == false) {
				return result;
			}
		}
	} else if(item.is_map()) {
		foreach(const variant_pair& p, item.as_map()) {
			variant result = get_map_editing(row, col, p.second);
			if(result.is_null() == false) {
				return result;
			}
		}

		return item;
	}

	return variant();
}

}

code_editor_widget::ObjectInfo code_editor_widget::get_object_at(int row, int col) const
{
	const int pos = row_col_to_text_pos(row, col);
	const char* ptr = current_text_.c_str() + pos;
	ASSERT_LOG(pos >= 0 && pos <= current_text_.size(), "Unexpected position in code editor widget: " << pos << " / " << current_text_.size());
	const json::Token* begin_token = NULL;
	const json::Token* end_token = NULL;
	std::stack<const json::Token*> begin_stack;
	int nbracket = 0;
	foreach(const json::Token& token, tokens_) {
		if(token.type == json::Token::TYPE_LCURLY) {
			begin_stack.push(&token);
		}

		if(token.type == json::Token::TYPE_RCURLY) {
			if(begin_stack.empty()) {
				return ObjectInfo();
			}

			if(begin_stack.top()->begin <= ptr && token.begin >= ptr) {
				begin_token = begin_stack.top();
				end_token = &token;
				break;
			} else {
				begin_stack.pop();
			}
		}
	}

	if(!begin_token || !end_token) {
		return ObjectInfo();
	}

	ObjectInfo result;
	result.begin = begin_token->begin - current_text_.c_str();
	result.end = end_token->end - current_text_.c_str();
	result.tokens = std::vector<json::Token>(begin_token, end_token+1);
	try {
		result.obj = get_map_editing(row, col, current_obj_);
	} catch(json::parse_error& e) {
		std::cerr << "json parse error: " << std::string(begin_token->begin, end_token->end) << "\n";
		return result;
	}

	return result;
}

code_editor_widget::ObjectInfo code_editor_widget::get_current_object() const
{
	return get_object_at(cursor_row(), cursor_col());
}

void code_editor_widget::set_highlight_current_object(bool value)
{
	if(!value) {
		clear_highlight_lines();
		return;
	}

	ObjectInfo info = get_current_object();
	if(info.obj.is_null() == false) {
		set_highlight_lines(text_pos_to_row_col(info.begin).first,
		                    text_pos_to_row_col(info.end).first);
	} else {
		clear_highlight_lines();
	}
}

void code_editor_widget::modify_current_object(variant new_obj)
{
	ObjectInfo info = get_current_object();
	if(info.obj.is_null() || info.tokens.empty()) {
		return;
	}

	save_undo_state();


	const std::string str(current_text_.begin() + info.begin, current_text_.begin() + info.end);

	//calculate the indentation this object has based on the first attribute.
	std::string indent;
	std::string::const_iterator end_line = std::find(str.begin(), str.end(), '\n');
	if(end_line != str.end()) {
		++end_line;
		std::string::const_iterator end_indent = end_line;
		while(end_indent != str.end() && util::c_isspace(*end_indent)) {
			if(*end_indent == '\n') {
				end_line = end_indent+1;
			}
			++end_indent;
		}

		indent = std::string(end_line, end_indent);
	}

	const std::string new_str = modify_variant_text(str, info.obj, new_obj, info.obj.get_debug_info()->line, info.obj.get_debug_info()->column, indent);
	current_text_ = std::string(current_text_.begin(), current_text_.begin() + info.begin) + new_str + std::string(current_text_.begin() + info.end, current_text_.end());
	set_text(current_text_, false /*don't move cursor*/);
}


}
