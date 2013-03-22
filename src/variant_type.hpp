#ifndef VARIANT_TYPE_HPP_INCLUDED
#define VARIANT_TYPE_HPP_INCLUDED

#include <boost/shared_ptr.hpp>

#include "formula_tokenizer.hpp"
#include "variant.hpp"

class variant_type;
typedef boost::shared_ptr<variant_type> variant_type_ptr;
typedef boost::shared_ptr<const variant_type> const_variant_type_ptr;

class variant_type
{
public:
	virtual ~variant_type() {}
	virtual bool match(const variant& v) const = 0;

	void set_str(const std::string& s) { str_ = s; }
	const std::string& str() const { return str_; }

private:
	std::string str_;
};

variant_type_ptr parse_variant_type(const variant& original_str,
                                    const formula_tokenizer::token*& i1,
                                    const formula_tokenizer::token* i2);
variant_type_ptr parse_variant_type(const variant& v);

#endif
