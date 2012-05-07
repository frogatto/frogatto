#include <algorithm>
#include <cstdint>

#include "asserts.hpp"
#include "colorshift_hash_table.hpp"

namespace {
	const int ArraySizes[] = {7, 17, 37, 79, 163, 331, 673, 1361, 2729, 5471, 10949, 21911, 43853, 87719, 175447, 350899, 701819, 1403641, 2807303, 5614657, 11229331, 22458671, 44917381, 89834777, };  //normally a vector would increase in size by ^2 with each iteration.  However, with our hashing function, this yields perversely bad collisions, because the last byte of each color value is the alpha, and generally is always opaque or transparent (e.g. 0x00 or 0xFF).  Whenever this lines lines up with a power of 2, such as with an arraySize of 256, it would fail like this.  Thus we're using primes, instead.  Our primes are also precalculated because in practice we'll never have many colors.
}

colorshift_hash_table::colorshift_hash_table()
  :	length_(10),
	elements_stored_(0),
	empty_color_(0x6f6d5100),
	num_predefined_sizes_(24)
{
	array_ = new pixel_pair[length_];
	ideal_size_ = length_ / 2;
	
	std::fill(array_, array_ + length_, std::pair<uint32_t, uint32_t> (empty_color_,empty_color_));
}

colorshift_hash_table::~colorshift_hash_table(){
	delete [] array_;
}

void colorshift_hash_table::insert(const pixel_pair& entry){
	
	if( elements_stored_ >= ideal_size_){
		colorshift_hash_table::grow_array();
	}
	
	int desired_element = entry.first%length_;
	
	for(int i=0; i <= elements_stored_; ++i){
		desired_element = (desired_element + 1) % length_; //modulo addition
		
		if (array_[desired_element].first != empty_color_){
			array_[desired_element] = entry;
			
			++elements_stored_;
			return;
		}
	}
}

void colorshift_hash_table::grow_array(){
	int new_length = 0;
	pixel_pair* temp_array; //copy of the old array
	pixel_pair* new_array; //new, bigger array
	
	
	for(int i=0; i < num_predefined_sizes_; ++i){
		if (length_ <= ArraySizes[i]){
			new_length = ArraySizes[i+1];
			break;
		}
	}
	ASSERT_NE(new_length,0);

	new_array = new pixel_pair[new_length];
	std::fill(new_array, new_array + new_length, std::pair<uint32_t, uint32_t> (empty_color_,empty_color_));
	
	//set aside address to old array
	temp_array = &array_[0];
	array_ = &new_array[0];
	
	ideal_size_ = new_length / 2;  //bad code smell:  failure to set this could result in an infinite recursion between this and insert
	
	for(int i=0; i<length_ ;++i){
		colorshift_hash_table::insert(temp_array[i]);
	}
	delete temp_array;
}


uint32_t colorshift_hash_table::operator[](uint32_t key) const {

	//If the first+n element isn't what we're looking for, then loop forward until we find an empty slot - because of how these are inserted, this is a faster way of indicating the array doesn't have the desired item than the alternative of looping over the entire array.
	int desired_element = key%length_;
	
	for(int i=0; i <= elements_stored_; ++i){
		desired_element = (desired_element + 1) % length_; //modulo addition
		
		if (array_[desired_element].first == key){  //then we don't have a collision, and can return the color
			return array_[key%length_].second;
		} else if (array_[desired_element].first == empty_color_) {   //if we did have a collision, we normally step to the next element, unless the one we're on is empty, which means the color isn't in our array
			return key; //identity operation
		}
	}	
		//if we manage to step through the entire array without reaching any empty elements, something is wrong and we should probably assert, however
	return key; //identity operation
}


