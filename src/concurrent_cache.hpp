#ifndef CONCURRENT_CACHE_HPP_INCLUDED
#define CONCURRENT_CACHE_HPP_INCLUDED

#include <map>
#include <vector>

#include "thread.hpp"

template<typename Key, typename Value>
class concurrent_cache
{
public:
	typedef std::map<Key, Value> map_type;
	size_t size() const { threading::lock l(mutex_); return map_.size(); }
	const Value& get(const Key& key) {
		threading::lock l(mutex_);
		typename map_type::const_iterator itor = map_.find(key);
		if(itor != map_.end()) {
			return itor->second;
		} else {
			static const Value empty_result = Value();
			return empty_result;
		}
	}

	void put(const Key& key, const Value& value) {
		threading::lock l(mutex_);
		map_[key] = value;
	}

	void erase(const Key& key) {
		threading::lock l(mutex_);
		map_.erase(key);
	}

	int count(const Key& key) const {
		threading::lock l(mutex_);
		return map_.count(key);
	}

	void clear() {
		threading::lock l(mutex_);
		map_.clear();
	}

	std::vector<Key> get_keys() {
		std::vector<Key> result;
		threading::lock l(mutex_);
		for(typename map_type::const_iterator i = map_.begin(); i != map_.end(); ++i) {
			result.push_back(i->first);
		}

		return result;
	}

	struct lock : public threading::lock {
		explicit lock(concurrent_cache& cache) : threading::lock(cache.mutex_), cache_(cache) {
		}

		map_type& map() const { return cache_.map_; }

	private:
		concurrent_cache& cache_;
	};

private:
	map_type map_;
	mutable threading::mutex mutex_;
};

#endif
