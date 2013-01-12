#include "asserts.hpp"
#include "foreach.hpp"
#include "string_utils.hpp"
#include "variant_utils.hpp"

game_logic::formula_callable_ptr map_into_callable(variant v)
{
	if(v.is_callable()) {
		return game_logic::formula_callable_ptr(v.mutable_callable());
	} else if(v.is_map()) {
		game_logic::map_formula_callable* res = new game_logic::map_formula_callable;
		foreach(const variant_pair& p, v.as_map()) {
			res->add(p.first.as_string(), p.second);
		}

		return game_logic::formula_callable_ptr(res);
	} else {
		return game_logic::formula_callable_ptr();
	}
}

variant append_variants(variant a, variant b)
{
	if(a.is_null()) {
		return b;
	} else if(b.is_null()) {
		return a;
	} else if(a.is_list()) {
		if(b.is_list()) {
			if(b.num_elements() > 0 && (b[0].is_numeric() || b[0].is_string()) ||
			   a.num_elements() > 0 && (a[0].is_numeric() || a[0].is_string())) {
				//lists of numbers or strings are treated like scalars and we
				//set the value of b.
				return b;
			}

			return a + b;
		} else {
			std::vector<variant> v(1, b);
			return a + variant(&v);
		}
	} else if(b.is_list()) {
		std::vector<variant> v(1, a);
		return variant(&v) + b;
	} else if(a.is_map() && b.is_map()) {
		std::vector<variant> v;
		v.push_back(a);
		v.push_back(b);
		return variant(&v);
	} else {
		return b;
	}
}

std::vector<std::string> parse_variant_list_or_csv_string(variant v)
{
	if(v.is_string()) {
		return util::split(v.as_string());
	} else if(v.is_list()) {
		return v.as_list_string();
	} else {
		ASSERT_LOG(v.is_null(), "Unexpected value when expecting a string list: " << v.write_json());
		return std::vector<std::string>();
	}
}

void merge_variant_over(variant* aptr, variant b)
{
	variant& a = *aptr;

	foreach(variant key, b.get_keys().as_list()) {
		a = a.add_attr(key, append_variants(a[key], b[key]));
	}
	
	if(!a.get_debug_info() && b.get_debug_info()) {
		a.set_debug_info(*b.get_debug_info());
	}
}

void visit_variants(variant v, boost::function<void (variant)> fn)
{
	fn(v);

	if(v.is_list()) {
		foreach(const variant& item, v.as_list()) {
			visit_variants(item, fn);
		}
	} else if(v.is_map()) {
		foreach(const variant_pair& item, v.as_map()) {
			visit_variants(item.second, fn);
		}
	}
}

variant deep_copy_variant(variant v)
{
	if(v.is_map()) {
		std::map<variant,variant> m;
		foreach(variant key, v.get_keys().as_list()) {
			m[key] = deep_copy_variant(v[key]);
		}

		return variant(&m);
	} else if(v.is_list()) {
		std::vector<variant> items;
		foreach(variant item, v.as_list()) {
			items.push_back(deep_copy_variant(item));
		}

		return variant(&items);
	} else {
		return v;
	}
}

variant_builder& variant_builder::add_value(const std::string& name, const variant& val)
{
	attr_[variant(name)].push_back(val);
	return *this;
}

variant_builder& variant_builder::set_value(const std::string& name, const variant& val)
{
	variant key(name);
	attr_.erase(key);
	attr_[key].push_back(val);
	return *this;
}

void variant_builder::merge_object(variant obj)
{
	foreach(variant key, obj.get_keys().as_list()) {
		set_value(key.as_string(), obj[key]);
	}
}

variant variant_builder::build()
{
	std::map<variant, variant> res;
	for(std::map<variant, std::vector<variant> >::iterator i = attr_.begin(); i != attr_.end(); ++i) {
		if(i->second.size() == 1) {
			res[i->first] = i->second[0];
		} else {
			res[i->first] = variant(&i->second);
		}
	}
	return variant(&res);
}

