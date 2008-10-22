#include <iostream>
#include <map>

#include "formula.hpp"
#include "formula_callable.hpp"
#include "formula_function.hpp"
#include "surface.hpp"
#include "surface_formula.hpp"

using namespace graphics;
using namespace game_logic;

namespace {

typedef std::pair<surface, std::string> cache_key;
typedef std::map<cache_key, surface> cache_map;
cache_map cache;

class rgba_function : public function_expression {
public:
	explicit rgba_function(surface surf, const args_list& args)
	  : function_expression("rgba", args, 4), surf_(surf) {
	}

private:
	variant execute(const formula_callable& variables) const {
		return variant(SDL_MapRGBA(surf_->format,
		   Uint8(args()[0]->evaluate(variables).as_int()),
		   Uint8(args()[1]->evaluate(variables).as_int()),
		   Uint8(args()[2]->evaluate(variables).as_int()),
		   Uint8(args()[3]->evaluate(variables).as_int())));
	}
	surface surf_;
};

class surface_formula_symbol_table : public function_symbol_table
{
public:
	explicit surface_formula_symbol_table(surface surf) : surf_(surf)
	{}
	expression_ptr create_function(
	                           const std::string& fn,
	                           const std::vector<expression_ptr>& args) const;
private:
	surface surf_;
};

expression_ptr surface_formula_symbol_table::create_function(
                           const std::string& fn,
                           const std::vector<expression_ptr>& args) const
{
	if(fn == "rgba") {
		return expression_ptr(new rgba_function(surf_, args));
	} else {
		return function_symbol_table::create_function(fn, args);
	}
}

class pixel_callable : public game_logic::formula_callable {
public:
	pixel_callable(const surface& surf, Uint32 pixel)
	{
		SDL_GetRGBA(pixel, surf->format, &r, &g, &b, &a);
		static const unsigned char AlphaPixel[] = {0x6f, 0x6d, 0x51};
		if(r == AlphaPixel[0] && g == AlphaPixel[1] && b == AlphaPixel[2]) {
			a = 0;
		}
	}

	bool is_alpha() const { return a == 0; }
private:
	variant get_value(const std::string& key) const {
		switch(*key.c_str()) {
		case 'r': return variant(r);
		case 'g': return variant(g);
		case 'b': return variant(b);
		case 'a': return variant(a);
		default: return variant();
		}
	}

	Uint8 r, g, b, a;
};

void run_formula(surface surf, const std::string& algo)
{
	const int ticks = SDL_GetTicks();
	surface_formula_symbol_table table(surf);
	game_logic::formula f(algo, &table);
	bool locked = false;
	if(SDL_MUSTLOCK(surf.get())) {
		const int res = SDL_LockSurface(surf.get());
		if(res == 0) {
			locked = true;
		}
	}

	std::map<Uint32, Uint32> pixel_map;

	Uint32* pixels = reinterpret_cast<Uint32*>(surf->pixels);
	Uint32* end_pixels = pixels + surf->w*surf->h;

	while(pixels != end_pixels) {
		pixel_callable p(surf, *pixels);
		if(p.is_alpha() == false) {
			std::map<Uint32, Uint32>::const_iterator itor = pixel_map.find(*pixels);
			if(itor == pixel_map.end()) {
				Uint32 result = f.execute(p).as_int();
				pixel_map[*pixels] = result;
				*pixels = result;
			} else {
				*pixels = itor->second;
			}
		}
		++pixels;
	}

	if(locked) {
		SDL_UnlockSurface(surf.get());
	}

	std::cerr << "run_formula in " << (SDL_GetTicks() - ticks) << " ticks\n";
}

}

surface get_surface_formula(surface input, const std::string& algo)
{
	if(algo.empty()) {
		return input;
	}

	cache_key key(input, algo);
	surface& surf = cache[key];
	if(surf.get() == NULL) {
		surf = input.clone();
		run_formula(surf, algo);
	}

	return surf;
}
