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
#pragma once
#ifndef DATA_BLOB_HPP_INCLUDED
#define DATA_BLOB_HPP_INCLUDED

#include <boost/shared_ptr.hpp>

#include "SDL.h"
#include "formula_callable.hpp"

class data_blob : public game_logic::formula_callable
{
public:
	data_blob(const std::string& key, const std::vector<char>& in_data) 
		: data_(in_data), key_(key)
	{
		rw_ops_ = boost::shared_ptr<SDL_RWops>(SDL_RWFromMem(&data_[0], data_.size()), deleter());
	}
	
	virtual ~data_blob()
	{
	}

	variant get_value(const std::string& key) const 
	{
		return variant();
	}

	SDL_RWops* get_rw_ops()
	{
		return rw_ops_.get();
	}

	std::string operator()()
	{
		return key_;
	}

private:
	struct deleter
	{
		void operator()(SDL_RWops* p) 
		{ 
			SDL_FreeRW(p);
		}
	};

	std::vector<char> data_;
	std::string key_;
	boost::shared_ptr<SDL_RWops> rw_ops_;
};

typedef boost::intrusive_ptr<data_blob> data_blob_ptr;
typedef boost::intrusive_ptr<const data_blob> const_data_blob_ptr;

#endif