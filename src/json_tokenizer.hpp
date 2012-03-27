#ifndef JSON_TOKENIZER_HPP_INCLUDED
#define JSON_TOKENIZER_HPP_INCLUDED

namespace json {

struct TokenizerError {
	const char* msg;
	const char* loc;
};

struct Token {
	enum TYPE { TYPE_NUMBER, TYPE_STRING, TYPE_LCURLY, TYPE_RCURLY,
	            TYPE_LSQUARE, TYPE_RSQUARE, TYPE_COMMA, TYPE_COLON,
	            TYPE_TRUE_VALUE, TYPE_FALSE_VALUE, TYPE_NULL_VALUE,
	            TYPE_IDENTIFIER,
	            NUM_TYPES };
	TYPE type;
	const char* begin, *end;
};

Token get_token(const char*& i1, const char* i2);

}

#endif
