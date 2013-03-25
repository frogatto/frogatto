/*
	Copyright (C) 2003-2013 by David White <davewx7@gmail.com>
	
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef FORMULA_TOKENIZER_HPP_INCLUDED
#define FORMULA_TOKENIZER_HPP_INCLUDED

#include <string.h>

#include <algorithm>
#include <string>
#include <vector>

namespace formula_tokenizer
{

typedef std::string::const_iterator iterator;

enum FFL_TOKEN_TYPE { TOKEN_OPERATOR, TOKEN_STRING_LITERAL,
                  TOKEN_CONST_IDENTIFIER,
		          TOKEN_IDENTIFIER, TOKEN_INTEGER, TOKEN_DECIMAL,
                  TOKEN_LPARENS, TOKEN_RPARENS,
				  TOKEN_LSQUARE, TOKEN_RSQUARE, 
				  TOKEN_LBRACKET, TOKEN_RBRACKET,
				  TOKEN_COMMA, TOKEN_SEMICOLON, TOKEN_COLON, 
				  TOKEN_WHITESPACE, TOKEN_KEYWORD,
				  TOKEN_COMMENT, TOKEN_POINTER, TOKEN_LEFT_POINTER,
				  TOKEN_PIPE,
				  TOKEN_INVALID  };

struct token {
	FFL_TOKEN_TYPE type;
	iterator begin, end;

	bool equals(const char* s) const { return end - begin == strlen(s) && std::equal(begin, end, s); }
};

token get_token(iterator& i1, iterator i2);

struct token_error {
	token_error(const std::string& m);
	std::string msg;
};

//A special interface for searching for and matching tokens.
class token_matcher {
public:
	token_matcher();
	explicit token_matcher(FFL_TOKEN_TYPE type);
	token_matcher& add(FFL_TOKEN_TYPE type);
	token_matcher& add(const std::string& str);

	bool match(const token& t) const;

	//Find the first matching token within the given range and return it.
	//Does not return tokens that are inside any kinds of brackets.
	bool find_match(const token*& i1, const token* i2) const;
private:
	std::vector<FFL_TOKEN_TYPE> types_;
	std::vector<std::string> str_;
};

}

#endif
