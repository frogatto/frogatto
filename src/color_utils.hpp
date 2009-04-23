#ifndef COLOR_UTILS_HPP_INCLUDED
#define COLOR_UTILS_HPP_INCLUDED

#include "SDL.h"

#include <string>
#include <vector>
#include "formula_callable.hpp"

SDL_Color string_to_color(const std::string& str);


namespace graphics {
class color : public game_logic::formula_callable
{
	public:
	uint8_t r() const {return c_.rgba[0]; }
	uint8_t g() const {return c_.rgba[1]; }
	uint8_t b() const {return c_.rgba[2]; }
	uint8_t a() const {return c_.rgba[3]; }
	uint32_t rgba() const {return convert_pixel_byte_order(c_).value; }
	//TODO: color to return string version
	
	color( uint8_t r, uint8_t g, uint8_t b, uint8_t a);
	color( uint32_t rgba);
	color( const std::string& str);
	
	variant get_value(const std::string& key) const;

	
	private:
		union PixelUnion {
			uint32_t value;
			uint8_t rgba[4];
		};

		PixelUnion c_;
		
	static PixelUnion convert_pixel_byte_order(PixelUnion p) {
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
		std::reverse(p.rgba, p.rgba + sizeof(p.rgba));
#endif
		return p;
	}
};
	
}
#endif
