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
#ifndef COMPRESS_HPP_INCLUDED
#define COMPRESS_HPP_INCLUDED

#include <vector>

#include "base64.hpp"
#include "formula_callable.hpp"
#include "variant.hpp"

namespace zip {

struct CompressionException {
	const char* msg;
};

std::vector<char> compress(const std::vector<char>& data, int compression_level=-1);
std::vector<char> decompress(const std::vector<char>& data);
std::vector<char> decompress_known_size(const std::vector<char>& data, int size);

class compressed_data : public game_logic::formula_callable {
	std::vector<char> data_;
public:
	compressed_data(const std::vector<char>& in_data, int compression_level) {
		data_ = compress(in_data, compression_level);
	}
	variant get_value(const std::string& key) const {
		if(key == "encode") {
			std::vector<char> v = base64::b64encode(data_);
			return variant(std::string(v.begin(), v.end()));
		} else if(key == "decompress") {
			std::vector<char> v = decompress(data_);
			return variant(std::string(v.begin(), v.end()));
		}
		return variant();
	}
};
typedef boost::intrusive_ptr<zip::compressed_data> compressed_data_ptr;

}

#endif // COMPRES_HPP_INCLUDED
