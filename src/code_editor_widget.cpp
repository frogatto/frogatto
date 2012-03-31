#include "asserts.hpp"
#include "code_editor_widget.hpp"
#include "formula_tokenizer.hpp"

namespace gui
{

code_editor_widget::code_editor_widget(int width, int height)
  : text_editor_widget(width, height)
{
}

void code_editor_widget::on_change()
{
	bracket_match_.clear();
	colors_.clear();
	colors_.resize(colors_.size()+1);
	const std::string s = text();
	std::string::const_iterator i = s.begin();
	while(i != s.end()) {
		if(*i == '"') {
			std::vector<std::vector<std::pair<int, int> > > opening_brackets;

			colors_.back().push_back(graphics::color(196, 196, 196));
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

						if(t.type == formula_tokenizer::TOKEN_OPERATOR && isalpha(*t.begin)) {
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

void code_editor_widget::select_token(const std::string& row, int& begin_row, int& end_row, int& begin_col, int& end_col) const
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
}

}
