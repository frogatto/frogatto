#include "color_utils.hpp"
#include <string>
#include <vector>
#include "string_utils.hpp"
#include "unit_test.hpp"

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

UNIT_TEST(color)
{
	//check that a value plugged into the color is returned when we
	//ask for it.
	CHECK_EQ(color(758492).rgba(), 758492);

	//check that an rgba value is equal to an expected int value.
	CHECK_EQ(color(255,255,0,255).rgba(), color(0xFFFF00FF).rgba());

	//check that parsing from a string works properly
	CHECK_EQ(color("255,128,255,128").rgba(), color(255,128,255,128).rgba());

	//it would be nice to be able to specify a string with three instead of
	//four values, and have it just assume 255 for the alpha.
	CHECK_EQ(color("255,128,255").rgba(), color(255,128,255,255).rgba());

	//check that we can get a value out of a color
	CHECK_EQ(color(255,128,64,0).get_value("g").as_int(), 128);
}
