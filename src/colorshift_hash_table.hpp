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
#ifndef COLORSHIFT_HASH_TABLE_HPP_INCLUDED
#define COLORSHIFT_HASH_TABLE_HPP_INCLUDED

//A quick data structure for doing color shifts of pixel art images.
#include <utility>
#include <boost/cstdint.hpp>


class colorshift_hash_table {
public:
	colorshift_hash_table();
	~colorshift_hash_table();
	typedef std::pair<uint32_t, uint32_t> pixel_pair;  //original and result colors
	void insert(const pixel_pair& entry);  //insert a new entry into the table
	uint32_t operator[](uint32_t key) const;  //look up an item in the table
	
private:
	pixel_pair* array_;
	int length_;  //size of the array
	int ideal_size_;  // half the array size.  We want half the array to be empty, to avoid having collisions.
	int elements_stored_;  //number of actual valid items in the array
	const int empty_color_;  //when this is the key, this given array slot is 'empty'

	
	const int num_predefined_sizes_;
	
	void grow_array();
	
	
};

#endif