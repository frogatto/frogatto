#ifndef FROGATTO_FUNCTIONAL_HPP_INCLUDED
#define FROGATTO_FUNCTIONAL_HPP_INCLUDED


#include <vector>

template<typename To, typename From, typename Fn>
std::vector<To> map_vector(const std::vector<From>& v, Fn fn) {
	std::vector<To> result;
	result.reserve(v.size());
	for(typename std::vector<From>::const_iterator i = v.begin(); i != v.end(); ++i) {
		result.push_back(fn(*i));
	}

	return result;
}

#endif
