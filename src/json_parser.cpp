#include <algorithm>

#include "filesystem.hpp"
#include "foreach.hpp"
#include "formatter.hpp"
#include "json_parser.hpp"
#include "json_tokenizer.hpp"
#include "preprocessor.hpp"
#include "unit_test.hpp"

namespace json {
parse_error::parse_error(const std::string& msg)
  : message(msg), line(-1), col(-1)
{
}

parse_error::parse_error(const std::string& msg, int line, int col)
  : message(msg), line(line), col(col)
{
}

std::string parse_error::error_message() const
{
	return formatter() << "PARSE ERROR: " << fname << ": line " << line << " col " << col << ": " << message;
}

namespace {
int get_line_num(const std::string& doc, int pos) {
	return 1 + std::count(doc.begin(), doc.begin() + pos, '\n');
}

int get_col_number(const std::string& doc, int pos) {
	const int init = pos;
	while(pos >= 0 && doc[pos] != '\n' && doc[pos] != '\r') {
		--pos;

	}

	return 1 + init - pos;
}

void escape_string(std::string& s) {
	for(size_t n = 0; n < s.size(); ++n) {
		if(s[n] == '\\') {
			s.erase(s.begin() + n);
		}
	}
}
}

#define CHECK_PARSE(cond, msg, pos) if(!(cond)) { int ln = get_line_num(doc, (pos)); int col = get_col_number(doc, (pos)); throw parse_error((msg), ln, col); }

namespace {
enum VAL_TYPE { VAL_NONE, VAL_OBJ, VAL_ARRAY };
struct JsonObject {
	explicit JsonObject(variant::debug_info debug_info) : type(VAL_NONE), is_base(false), require_comma(false), require_colon(false), info(debug_info) {}
	std::map<variant, variant> obj;
	std::vector<variant> array;
	VAL_TYPE type;
	variant name;

	variant base;
	bool is_base;

	bool require_comma;
	bool require_colon;

	variant::debug_info info;

	void add(variant name, variant v) {
		if(type == VAL_OBJ) {
			obj[name] = v;
		} else {
			array.push_back(v);
		}
	}

	variant as_variant() {
		if(type == VAL_OBJ) {
			variant v(&obj);
			v.set_debug_info(info);
			return v;
		} else {
			variant v(&array);
			v.set_debug_info(info);
			return v;
		}
	}
};

std::set<std::string> filename_registry;

variant parse_internal(const std::string& doc, const std::string& fname,
                       JSON_PARSE_OPTIONS options)
{
	const bool use_preprocessor = options&JSON_USE_PREPROCESSOR;

	std::set<std::string>::const_iterator filename_itor = filename_registry.insert(fname).first;


	const std::string* filename = &*filename_itor;

	variant::debug_info debug_info;
	debug_info.filename = &*filename_itor;
	debug_info.line = 1;
	debug_info.column = 1;

	const char* debug_pos = doc.c_str();

	const char* i1 = doc.c_str();
	const char* i2 = i1 + doc.size();
	try {
		std::vector<JsonObject> stack;
		stack.push_back(JsonObject(debug_info));
		stack.push_back(JsonObject(debug_info));
		stack[0].type = VAL_ARRAY;

		for(Token t = get_token(i1, i2); t.type != Token::NUM_TYPES; t = get_token(i1, i2)) {
			while(debug_pos != t.begin) {
				if(*debug_pos == '\n') {
					++debug_info.line;
					debug_info.column = 0;
				} else {
					++debug_info.column;
				}

				++debug_pos;
			}

			CHECK_PARSE(stack.size() > 1, "Unexpected characters at end of input", t.begin - doc.c_str());

			CHECK_PARSE(!stack.back().require_colon || t.type == Token::TYPE_COLON, "Unexpected characters, when expecting a ':'", t.begin - doc.c_str());
			CHECK_PARSE(!stack.back().require_comma || t.type == Token::TYPE_COMMA || t.type == Token::TYPE_RCURLY || t.type == Token::TYPE_RSQUARE, "Unexpected characters, when expecting a ','", t.begin - doc.c_str());

			switch(t.type) {
			case Token::TYPE_COLON: {
				CHECK_PARSE(stack.back().require_colon, "Unexpected :", t.begin - doc.c_str());
				stack.back().require_colon = false;
				break;
			}

			case Token::TYPE_COMMA: {
				CHECK_PARSE(stack.back().require_comma, "Unexpected ,", t.begin - doc.c_str());
				stack.back().require_comma = false;
				break;
			}

			case Token::TYPE_LCURLY: {
				if(stack.back().type == VAL_ARRAY) {
					stack.push_back(JsonObject(debug_info));
				}

				CHECK_PARSE(stack.back().type == VAL_NONE, "Unexpected {", t.begin - doc.c_str());
				stack.back().type = VAL_OBJ;
				break;
			}
			case Token::TYPE_RCURLY: {
				CHECK_PARSE(stack.back().type == VAL_OBJ, "Unexpected }", t.begin - doc.c_str());

				variant name = stack.back().name;
				variant v = stack.back().as_variant();
				stack.pop_back();
				stack.back().add(name, v);
				stack.back().require_comma = true;
				break;
			}

			case Token::TYPE_LSQUARE: {
				if(stack.back().type == VAL_ARRAY) {
					stack.push_back(JsonObject(debug_info));
				}

				CHECK_PARSE(stack.back().type == VAL_NONE, "Unexpected [", t.begin - doc.c_str());
				stack.back().type = VAL_ARRAY;
				break;
			}

			case Token::TYPE_RSQUARE: {
				CHECK_PARSE(stack.back().type == VAL_ARRAY, "Unexpected ]", t.begin - doc.c_str());
				variant name = stack.back().name;
				variant v = stack.back().as_variant();
				stack.pop_back();
				stack.back().add(name, v);
				stack.back().require_comma = true;
				break;
			}

			case Token::TYPE_STRING:
			case Token::TYPE_IDENTIFIER: {
				std::string s(t.begin, t.end);

				if(t.type == Token::TYPE_STRING) {
					escape_string(s);
				}

				variant v;
				
				if(use_preprocessor) {
					try {
						v = preprocess_string_value(s);
					} catch(preprocessor_error& e) {
						CHECK_PARSE(false, "Preprocessor error: " + s, t.begin - doc.c_str());
					}

					if(stack.back().type == VAL_OBJ && stack[stack.size()-2].type == VAL_ARRAY && s == "@base") {
						stack.back().is_base = true;
					}
				} else {
					v = variant(s);
				}

				if(stack.back().type == VAL_OBJ) {
					stack.push_back(JsonObject(debug_info));
					v.set_debug_info(debug_info);
					stack.back().name = v;
					stack.back().require_colon = true;
				} else if(stack.back().type == VAL_ARRAY) {
					stack.back().add(variant(""), v);
					stack.back().require_comma = true;
				} else {
					variant name = stack.back().name;
					v.set_debug_info(debug_info);
					stack.pop_back();
					stack.back().add(name, v);
					stack.back().require_comma = true;
				}

				break;
			}

			case Token::TYPE_NUMBER:
			case Token::TYPE_TRUE_VALUE:
			case Token::TYPE_FALSE_VALUE:
			case Token::TYPE_NULL_VALUE: {
				variant v;
				if(t.type == Token::TYPE_NUMBER) {
					std::string s(t.begin, t.end);
					if(std::count(s.begin(), s.end(), '.')) {
							v = variant(decimal::from_string(s));
					} else {
						int val = atoi(s.c_str());
						v = variant(val);
					}
				} else if(t.type == Token::TYPE_TRUE_VALUE) {
					v = variant::from_bool(true);
				} else if(t.type == Token::TYPE_FALSE_VALUE) {
					v = variant::from_bool(false);
				}

				CHECK_PARSE(stack.back().type != VAL_OBJ, "Unexpected value in object", t.begin - doc.c_str());
				if(stack.back().type == VAL_ARRAY) {
					stack.back().add(variant(""), v);
					stack.back().require_comma = true;
				} else {
					variant name = stack.back().name;
					stack.pop_back();
					stack.back().add(name, v);
					stack.back().require_comma = true;
				}

				break;
			}
				
			default: assert(false);
			}
		}


		CHECK_PARSE(stack.size() == 1 && stack.back().array.size() == 1, "Unexpected end of input", i1 - doc.c_str());
		return stack.back().array.front();
	} catch(TokenizerError& e) {
		CHECK_PARSE(false, e.msg, e.loc - doc.c_str());
	}
}
}

variant parse(const std::string& doc, JSON_PARSE_OPTIONS options)
{
	return parse_internal(doc, "", options);
}

variant parse_from_file(const std::string& fname, JSON_PARSE_OPTIONS options)
{
	try {
		std::string data = sys::read_file(fname);
		if(data.empty()) {
			throw parse_error(formatter() << "File " << fname << " could not be read");
		}

		variant result = parse_internal(data, fname, options);
		return result;
	} catch(parse_error& e) {
		e.fname = fname;
		throw(e);
	}
}

}

