#ifndef COLOR_UTILS_HPP_INCLUDED
#define COLOR_UTILS_HPP_INCLUDED

#include "graphics.hpp"

#include <string>
#include <vector>
#include "formula_callable.hpp"

#include "variant.hpp"

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

	uint32_t value() const { return c_.value; }
	
	color( int r, int g, int b, int a=255);
	explicit color(uint32_t rgba = 0);
	explicit color(const std::string& str);
	explicit color(const variant& v);

	variant write() const;
	
	variant get_value(const std::string& key) const;

	void set_as_current_color() const;
	void add_to_vector(std::vector<GLfloat>* v) const;

	static uint32_t convert_pixel_byte_order(uint32_t p) {
		PixelUnion pu;
		pu.value = p;
		return convert_pixel_byte_order(pu).value;
	}

	SDL_Color as_sdl_color() const;

	bool operator<(const graphics::color& c) const { return value() < c.value(); }
	bool operator==(const graphics::color& c) const { return value() == c.value(); }

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

class color_transform : public game_logic::formula_callable
{
public:
	color_transform(const color& c);
	color_transform(int16_t r, int16_t g, int16_t b, int16_t a);
	explicit color_transform(const std::string& str);
	explicit color_transform(const variant& v);

	variant write() const;
	std::string to_string() const;

	int16_t r() const { return rgba_[0]; }
	int16_t g() const { return rgba_[1]; }
	int16_t b() const { return rgba_[2]; }
	int16_t a() const { return rgba_[3]; }

	int16_t* buf() { return rgba_; }
	const int16_t* buf() const { return rgba_; }
	
	variant get_value(const std::string& key) const;

	color to_color() const;

	//whether this fits in a color object.
	bool fits_in_color() const;
private:
	int16_t rgba_[4];
};

color_transform operator+(const color_transform& a, const color_transform& b);
color_transform operator-(const color_transform& a, const color_transform& b);

}
#endif
