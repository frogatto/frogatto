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
#ifndef JSON_TOKENIZER_HPP_INCLUDED
#define JSON_TOKENIZER_HPP_INCLUDED

namespace json {

struct TokenizerError {
	const char* msg;
	const char* loc;
};

struct Token {
	Token() : translate(false) {}
	enum TYPE { TYPE_NUMBER, TYPE_STRING, TYPE_LCURLY, TYPE_RCURLY,
	            TYPE_LSQUARE, TYPE_RSQUARE, TYPE_COMMA, TYPE_COLON,
	            TYPE_TRUE_VALUE, TYPE_FALSE_VALUE, TYPE_NULL_VALUE,
	            TYPE_IDENTIFIER,
	            NUM_TYPES };
	TYPE type;
	const char* begin, *end;
	bool translate;
};

Token get_token(const char*& i1, const char* i2);

//Gets the full token, unlike get_token which will e.g. return the
//characters inside the string.
Token get_token_full(const char*& i1, const char* i2);


}

#endif
