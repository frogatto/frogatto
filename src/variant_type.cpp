#include <vector>

#include "asserts.hpp"
#include "foreach.hpp"
#include "formula_object.hpp"
#include "formula_tokenizer.hpp"
#include "unit_test.hpp"
#include "variant_type.hpp"

namespace {

class variant_type_simple : public variant_type
{
public:
	variant_type_simple(const variant& original_str, const formula_tokenizer::token& tok)
	  : type_(variant::string_to_type(std::string(tok.begin, tok.end)))
	{
		ASSERT_LOG(type_ != variant::VARIANT_TYPE_INVALID, "INVALID TYPE: " << std::string(tok.begin, tok.end) << " AT " << original_str.debug_location());
	}

	bool match(const variant& v) const {
		return v.type() == type_ || type_ == variant::VARIANT_TYPE_DECIMAL && v.type() == variant::VARIANT_TYPE_INT;
	}
private:
	variant::TYPE type_;
};

class variant_type_class : public variant_type
{
public:
	explicit variant_type_class(const std::string& type) : type_(type)
	{
		ASSERT_LOG(game_logic::formula_class_valid(type), "INVALID FORMULA CLASS: " << type);
	}

	bool match(const variant& v) const {
		const game_logic::formula_object* obj = v.try_convert<game_logic::formula_object>();
		if(!obj) {
			return false;
		}

		return obj->is_a(type_);
	}
private:
	std::string type_;
};

class variant_type_union : public variant_type
{
public:
	explicit variant_type_union(const std::vector<variant_type_ptr>& v) : types_(v)
	{}
	bool match(const variant& v) const {
		foreach(const variant_type_ptr& p, types_) {
			if(p->match(v)) {
				return true;
			}
		}

		return false;
	}
private:
	std::vector<variant_type_ptr> types_;
};

class variant_type_list : public variant_type
{
public:
	explicit variant_type_list(const variant_type_ptr& value) : value_type_(value)
	{}

	bool match(const variant& v) const {
		if(!v.is_list()) {
			return false;
		}

		for(int n = 0; n != v.num_elements(); ++n) {
			if(!value_type_->match(v[n])) {
				return false;
			}
		}

		return true;
	}
private:
	variant_type_ptr value_type_;
};

class variant_type_map : public variant_type
{
public:
	variant_type_map(variant_type_ptr key, variant_type_ptr value)
	  : key_type_(key), value_type_(value)
	{}

	bool match(const variant& v) const {
		if(!v.is_map()) {
			return false;
		}

		foreach(const variant::map_pair& p, v.as_map()) {
			if(!key_type_->match(p.first) || !value_type_->match(p.second)) {
				return false;
			}
		}

		return true;
	}
private:
	variant_type_ptr key_type_, value_type_;
};

}

variant_type_ptr parse_variant_type(const variant& original_str,
                                    const formula_tokenizer::token*& i1,
                                    const formula_tokenizer::token* i2)
{
	using namespace formula_tokenizer;

	std::vector<variant_type_ptr> v;

	const token* begin_token = i1;

	for(;;) {
		ASSERT_LOG(i1 != i2, "EXPECTED TYPE BUT FOUND EMPTY EXPRESSION: " << original_str.debug_location());
		if(i1->type == TOKEN_IDENTIFIER && std::equal(i1->begin, i1->end, "class")) {
			++i1;
			ASSERT_LOG(i1 != i2, "EXPECTED CLASS BUT FOUND EMPTY EXPRESSION: " << original_str.debug_location());
			v.push_back(variant_type_ptr(new variant_type_class(std::string(i1->begin, i1->end))));

			++i1;
		} else if(i1->type == TOKEN_IDENTIFIER || (i1->type == TOKEN_KEYWORD && std::equal(i1->begin, i1->end, "null"))) {
			v.push_back(variant_type_ptr(new variant_type_simple(original_str, *i1)));
			++i1;
		} else if(i1->type == TOKEN_LBRACKET) {
			const token* end = i1+1;
			const bool res = token_matcher().add(TOKEN_RBRACKET).find_match(end, i2);
			ASSERT_LOG(res, "ERROR PARSING MAP TYPE: " << original_str.debug_location());

			++i1;
			ASSERT_LOG(i1 != end, "ERROR PARSING MAP TYPE: " << original_str.debug_location());

			const variant_type_ptr key_type = parse_variant_type(original_str, i1, end);
			ASSERT_LOG(i1->type == TOKEN_POINTER, "ERROR PARSING MAP TYPE, NO ARROW FOUND: " << original_str.debug_location());
		
			++i1;
			ASSERT_LOG(i1 != end, "ERROR PARSING MAP TYPE: " << original_str.debug_location());

			const variant_type_ptr value_type = parse_variant_type(original_str, i1, end);
			ASSERT_LOG(i1 == end, "ERROR PARSING MAP TYPE: " << original_str.debug_location());

			v.push_back(variant_type_ptr(new variant_type_map(key_type, value_type)));

			++i1;

		} else if(i1->type == TOKEN_LSQUARE) {
			const token* end = i1+1;
			const bool res = token_matcher().add(TOKEN_RSQUARE).find_match(end, i2);
			ASSERT_LOG(res, "ERROR PARSING ARRAY TYPE: " << original_str.debug_location());
	
			++i1;
			ASSERT_LOG(i1 != end, "ERROR PARSING ARRAY TYPE: " << original_str.debug_location());
			
			const variant_type_ptr value_type = parse_variant_type(original_str, i1, end);
			ASSERT_LOG(i1 == end, "ERROR PARSING ARRAY TYPE: " << original_str.debug_location());
	
			v.push_back(variant_type_ptr(new variant_type_list(value_type)));
	
			++i1;
		} else {
			ASSERT_LOG(false, "UNEXPECTED TOKENS WHEN PARSING TYPE: " << std::string(i1->begin, (i2-1)->end) << " AT " << original_str.debug_location());
		}

		if(i1 != i2 && i1->type == TOKEN_PIPE) {
			++i1;
		} else {
			break;
		}
	}

	if(v.size() == 1) {
		v.front()->set_str(std::string(begin_token->begin, (i1-1)->end));
		return v.front();
	} else {
		variant_type_ptr result(new variant_type_union(v));
		result->set_str(std::string(begin_token->begin, (i1-1)->end));
		return result;
	}
}

variant_type_ptr parse_variant_type(const variant& type)
{
	using namespace formula_tokenizer;
	const std::string& s = type.as_string();
	std::vector<token> tokens;
	std::string::const_iterator i1 = s.begin();
	std::string::const_iterator i2 = s.end();
	while(i1 != i2) {
		try {
			tokens.push_back(get_token(i1, i2));
		} catch(token_error& e) {
			ASSERT_LOG(false, "ERROR PARSING TYPE: " << e.msg << " IN '" << s << "' AT " << type.debug_location());
		}
	}

	ASSERT_LOG(tokens.empty() == false, "ERROR PARSING TYPE: EMPTY STRING AT " << type.debug_location());

	const token* begin = &tokens[0];
	return parse_variant_type(type, begin, begin + tokens.size());
}
