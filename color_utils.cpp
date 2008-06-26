#include "color_utils.hpp"

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
