#ifndef VARIANT_UTILS_HPP_INCLUDED
#define VARIANT_UTILS_HPP_INCLUDED

#include <boost/function.hpp>

#include <map>

#include "foreach.hpp"
#include "variant.hpp"

//Function which will try to append list of variations. Behaviors:
//anya, nullb -> anya
//nulla, anyb -> anyb
//lista, listb -> lista + listb
//anya, listb -> [anya] + listb
//lista, anyb -> lista + [anyb]
//obja, objb -> [obja, objb]
//scalara, scalarb -> scalarb
//lista, scalarb -> lista + [scalarb]
variant append_variants(variant a, variant b);

void merge_variant_over(variant* aptr, variant b);

void visit_variants(variant v, boost::function<void (variant)> fn);

template<typename Seq>
variant vector_to_variant(const Seq& seq) {
	std::vector<variant> v;
	foreach(const typename Seq::value_type& value, seq) {
		v.push_back(variant(value));
	}

	return variant(&v);
}

class variant_builder {
public:

	template<typename T>
	variant_builder& add(const std::string& name, const T& val) {
		return add_value(name, variant(val));
	}
	template<typename T>
	variant_builder& add(const std::string& name, T& val) {
		return add_value(name, variant(val));
	}
	variant_builder& add_value(const std::string& name, const variant& val);

	template<typename T>
	variant_builder& set(const std::string& name, const T& val) {
		return set_value(name, variant(val));
	}
	template<typename T>
	variant_builder& set(const std::string& name, T& val) {
		return set_value(name, variant(val));
	}
	variant_builder& set_value(const std::string& name, const variant& val);

	void merge_object(variant obj);

	variant build();
private:
	std::map<variant, std::vector<variant> > attr_;
};
#endif
