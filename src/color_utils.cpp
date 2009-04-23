#include "color_utils.hpp"
#include <string>
#include <vector>
#include "string_utils.hpp"


SDL_Color string_to_color(const std::string& str)
{
	SDL_Color res = {0,0,0,0};
	if(str.size() == 6) {
		unsigned int num = strtol(str.c_str(), NULL, 16);
		res.r = (num >> 16)&0xFF;
		res.g = (num >> 8)&0xFF;
		res.b = num&0xFF;
	}
	return res;
}

using namespace graphics;

color::color( uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	c_.rgba[0] = r;
	c_.rgba[1] = g;
	c_.rgba[2] = b;
	c_.rgba[3] = a;
	
}

color::color( uint32_t rgba)
{
	c_.value = rgba;
}

color::color( const std::string& str)
{
	std::vector<std::string> components;
	components = util::split(str);
	
	if( components.size() == 4){
		int i=0;
		for( std::vector<std::string>::iterator itor = components.begin();
			itor != components.end(); ++itor)
		{
			c_.rgba[i] = atoi(itor->c_str());
		}
	}
}


variant color::get_value(const std::string& key) const
{
	if(key == "r"){
		return variant(r());
	} else if(key == "g"){
		return variant(g());
	} else if(key == "b"){
		return variant(b());
	} else if(key == "a"){
		return variant(a());
	} else {
		return variant();
	}
}

