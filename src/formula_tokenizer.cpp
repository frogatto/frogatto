/* $Id: formula_tokenizer.cpp 25713 2008-04-09 18:36:16Z dragonking $ */
/*
   Copyright (C) 2007 - 2008 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

#include <iostream>

#include <boost/regex.hpp>

#include "foreach.hpp"
#include "formula_tokenizer.hpp"
#include "unit_test.hpp"

namespace formula_tokenizer
{

namespace {

using boost::regex;

struct token_type {
	regex re;
	TOKEN_TYPE type;
};

//create the array with list of possible tokens
token_type token_types[] = { { regex("^(not\\b|and\\b|or\\b|where\\b|in\\b|d(?=[^a-zA-Z])|\\*|\\+|-(?=[^>])|\\^|%|/|<=|>=|<|>|!=|=|\\.)"), TOKEN_OPERATOR },
				{ regex("^functions\\b"),  TOKEN_KEYWORD },
				{ regex("^def\\b"),        TOKEN_KEYWORD },
				{ regex("^'[^']*'"),       TOKEN_STRING_LITERAL },
				{ regex("^~[^~]*~"),       TOKEN_STRING_LITERAL },
				{ regex("^[A-Z][A-Z_0-9]*"),    TOKEN_CONST_IDENTIFIER },
				{ regex("^[a-zA-Z_][a-zA-Z_0-9]*"),    TOKEN_IDENTIFIER },
				{ regex("^0x[0-9a-fA-F]+"),          TOKEN_INTEGER },
				{ regex("^\\d+"),          TOKEN_INTEGER },
				{ regex("^\\("),           TOKEN_LPARENS },
				{ regex("^\\)"),           TOKEN_RPARENS },
				{ regex("^\\["),           TOKEN_LSQUARE },
				{ regex("^\\]"),           TOKEN_RSQUARE },
				{ regex("^\\{"),           TOKEN_LBRACKET },
				{ regex("^\\}"),           TOKEN_RBRACKET },
				{ regex("^#.*?#"),	   TOKEN_COMMENT },
				{ regex("^,"),             TOKEN_COMMA },
				{ regex("^;"),             TOKEN_SEMICOLON },
				{ regex("^\\s+"),          TOKEN_WHITESPACE },
				{ regex("^->"),          TOKEN_POINTER }
};

}

token get_token(iterator& i1, iterator i2) {
	foreach(const token_type& t, token_types) {
		boost::smatch match;
		if(boost::regex_search(i1, i2, match, t.re, boost::match_single_line)) {
			token res;
			res.type = t.type;
			res.begin = i1;
			i1 = res.end = i1 + match.length();
			
			return res;
		}
	}

	std::cerr << "Unrecognized token: '" << std::string(i1,i2) << "'\n";
	throw token_error();
}

}

UNIT_TEST(tokenizer_test)
{
	using namespace formula_tokenizer;
	std::string test = "(abc + 0x4 * (5+3))*2 in [4,5]";
	std::string::const_iterator i1 = test.begin();
	std::string::const_iterator i2 = test.end();
	TOKEN_TYPE types[] = {TOKEN_LPARENS, TOKEN_IDENTIFIER,
	                      TOKEN_WHITESPACE, TOKEN_OPERATOR,
						  TOKEN_WHITESPACE, TOKEN_INTEGER,
						  TOKEN_WHITESPACE, TOKEN_OPERATOR,
						  TOKEN_WHITESPACE, TOKEN_LPARENS,
						  TOKEN_INTEGER, TOKEN_OPERATOR,
						  TOKEN_INTEGER, TOKEN_RPARENS,
						  TOKEN_RPARENS, TOKEN_OPERATOR, TOKEN_INTEGER};
	std::string tokens[] = {"(", "abc", " ", "+", " ", "0x4", " ",
	                        "*", " ", "(", "5", "+", "3", ")", ")", "*", "2",
							"in", "[", "4", ",", "5", "]"};
	for(int n = 0; n != sizeof(types)/sizeof(*types); ++n) {
		token t = get_token(i1,i2);
		CHECK_EQ(std::string(t.begin,t.end), tokens[n]);
		CHECK_EQ(t.type, types[n]);

	}
}
