#include <map>
#include <vector>

#include "asserts.hpp"
#include "surface_cache.hpp"
#include "surface_palette.hpp"

namespace graphics
{

namespace {

struct palette_definition {
	std::string name;
	std::map<uint32_t, uint32_t> mapping;
};

std::vector<palette_definition> palettes;

void load_palette_def(const std::string& id)
{
	palette_definition def;
	def.name = id;
	surface s = surface_cache::get_no_cache("palette/" + id + ".png");

	surface converted(SDL_CreateRGBSurface(SDL_SWSURFACE, s->w, s->h, 32, SURFACE_MASK));
	SDL_SetAlpha(s.get(), 0, SDL_ALPHA_OPAQUE);
	SDL_BlitSurface(s.get(), NULL, converted.get(), NULL);
	s = converted;

	ASSERT_LOG(s.get(), "COULD NOT LOAD PALETTE IMAGE " << id);
	ASSERT_LOG(s->format->BytesPerPixel == 4, "PALETTE " << id << " NOT IN 32bpp PIXEL FORMAT");

	const uint32_t* pixels = reinterpret_cast<const uint32_t*>(s->pixels);
	for(int n = 0; n < s->w*s->h - 1; n += 2) {
		def.mapping.insert(std::pair<uint32_t,uint32_t>(pixels[0], pixels[1]));
		pixels += 2;
	}

	palettes.push_back(def);
}

}

int get_palette_id(const std::string& name)
{
	if(name.empty()) {
		return -1;
	}

	static std::map<std::string, int> m;
	std::map<std::string, int>::const_iterator i = m.find(name);
	if(i != m.end()) {
		return i->second;
	}

	const int id = m.size();
	m[name] = id;
	load_palette_def(name);
	return get_palette_id(name);
}

const std::string& get_palette_name(int id)
{
	if(id < 0 || id >= palettes.size()) {
		static const std::string str;
		return str;
	} else {
		return palettes[id].name;
	}
}

surface map_palette(surface s, int palette)
{
	if(palette < 0 || palette >= palettes.size() || palettes[palette].mapping.empty()) {
		return s;
	}

	surface result(SDL_CreateRGBSurface(SDL_SWSURFACE, s->w, s->h, 32, SURFACE_MASK));

	s = surface(SDL_ConvertSurface(s.get(), result->format, 0));

	ASSERT_LOG(s->format->BytesPerPixel == 4, "SURFACE NOT IN 32bpp PIXEL FORMAT");

	std::cerr << "mapping palette " << palette << "\n";


	uint32_t* dst = reinterpret_cast<uint32_t*>(result->pixels);
	const uint32_t* src = reinterpret_cast<const uint32_t*>(s->pixels);

	const std::map<uint32_t,uint32_t>& mapping = palettes[palette].mapping;

	for(int n = 0; n != s->w*s->h; ++n) {
		std::map<uint32_t,uint32_t>::const_iterator i = mapping.find(*src);
		if(i != mapping.end()) {
			*dst = i->second;
		} else {
			*dst = *src;
		}

		++src;
		++dst;
	}
	return result;
}

color map_palette(const color& c, int palette)
{
	if(palette < 0 || palette >= palettes.size() || palettes[palette].mapping.empty()) {
		return c;
	}

	const std::map<uint32_t,uint32_t>& mapping = palettes[palette].mapping;
	std::map<uint32_t,uint32_t>::const_iterator i = mapping.find(c.value());
	if(i != mapping.end()) {
		return color(color::convert_pixel_byte_order(i->second));
	} else {
		return c;
	}
}

SDL_Color map_palette(const SDL_Color& c, int palette)
{
	color result = map_palette(color(c.r, c.g, c.b, 255), palette);
	SDL_Color res = { result.r(), result.g(), result.b(), 255 };
	return res;
}

}
