/*
	Copyright (C) 2003-2013 by David White <davewx7@gmail.com>
	
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
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
