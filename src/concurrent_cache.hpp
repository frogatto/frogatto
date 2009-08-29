#ifndef CONCURRENT_CACHE_HPP_INCLUDED
#define CONCURRENT_CACHE_HPP_INCLUDED

#include <map>

#include "thread.hpp"

template<typename Key, typename Value>
class concurrent_cache
{
	typedef std::map<Key, Value> map_type;
	map_type map_;
	threading::mutex mutex_;
public:
	Value get(const Key& key) {
		threading::lock l(mutex_);
		typename map_type::const_iterator itor = map_.find(key);
		if(itor != map_.end()) {
			return itor->second;
		} else {
			return Value();
		}
	}

	void put(const Key& key, const Value& value) {
		threading::lock l(mutex_);
		map_.insert(std::pair<Key,Value>(key, value));
	}

	void clear() {
		threading::lock l(mutex_);
		map_.clear();
	}
};

#endif
