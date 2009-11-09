#include <iostream>
#include <map>
#include <GL/glew.h>

#include "concurrent_cache.hpp"
#include "filesystem.hpp"
#include "formula.hpp"
#include "formula_callable.hpp"
#include "formula_function.hpp"
#include "hi_res_timer.hpp"
#include "surface.hpp"
#include "surface_cache.hpp"
#include "surface_formula.hpp"
#include "unit_test.hpp"

using namespace graphics;
using namespace game_logic;

namespace {

typedef std::pair<surface, std::string> cache_key;
typedef concurrent_cache<cache_key, surface> cache_map;
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
	const hi_res_timer timer("run_formula");

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

	Uint32 AlphaPixel = SDL_MapRGBA(surf->format, 0x6f, 0x6d, 0x51, 0x0);

	int skip = 0;
	while(pixels != end_pixels) {
		if(((*pixels)&(~surf->format->Amask)) == AlphaPixel) {
			++pixels;
			continue;
		}
		std::map<Uint32, Uint32>::const_iterator itor = pixel_map.find(*pixels);
		if(itor == pixel_map.end()) {
			pixel_callable p(surf, *pixels);
			Uint32 result = f.execute(p).as_int();
			pixel_map[*pixels] = result;
			*pixels = result;
		} else {
			*pixels = itor->second;
		}
		++pixels;
	}

	if(locked) {
		SDL_UnlockSurface(surf.get());
	}
}

}

surface get_surface_formula(surface input, const std::string& algo)
{
	if(algo.empty()) {
		return input;
	}

	cache_key key(input, algo);
	surface surf = cache.get(key);
	if(surf.get() == NULL) {
		surf = input.clone();
		run_formula(surf, algo);
		cache.put(key, surf);
	}

	return surf;
}

namespace {
typedef std::map<std::pair<std::string,std::string>, GLuint> shader_map;
shader_map shader_cache;
}

GLuint get_gl_shader(const std::string& vertex_shader_file, const std::string& fragment_shader_file)
{
	shader_map::iterator itor = shader_cache.find(std::make_pair(vertex_shader_file, fragment_shader_file));
	if(itor != shader_cache.end()) {
		return itor->second;
	}

	const std::string vertex_shader = sys::read_file("data/shaders/" + vertex_shader_file);
	const std::string fragment_shader = sys::read_file("data/shaders/" + fragment_shader_file);

	GLuint vertex_id = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragment_id = glCreateShader(GL_FRAGMENT_SHADER);

	const char* vertex_str = vertex_shader.c_str();
	glShaderSource(vertex_id, 1, &vertex_str, NULL);

	const char* fragment_str = fragment_shader.c_str();
	glShaderSource(fragment_id, 1, &fragment_str, NULL);

	glCompileShader(vertex_id);
	glCompileShader(fragment_id);

	GLuint program_id = glCreateProgram();
	glAttachShader(program_id, vertex_id);
	glAttachShader(program_id, fragment_id);
	glLinkProgram(program_id);

	shader_cache[std::make_pair(vertex_shader_file, fragment_shader_file)] = program_id;

	glUseProgram(0);

	return program_id;
}

BENCHMARK(surface_formula)
{
	surface s(graphics::surface_cache::get("characters/frogatto-spritesheet1.png"));
	assert(s.get());

	surface target(SDL_CreateRGBSurface(SDL_SWSURFACE,s->w,s->h,32,SURFACE_MASK));
	SDL_BlitSurface(s.get(), NULL, target.get(), NULL);

	const std::string algo("rgba(b,r,g,a)");
	BENCHMARK_LOOP {
		run_formula(target, algo);
	}
}

BENCHMARK(pixel_table)
{
	//This is some hard coded test data. It gives the set of pixels in
	//the input image, and the pixels we want to map to.
	const Uint32 PixelsFrom[] = {0xFF00FFFF, 0xFFFFFFFF, 0x9772FF13, 0xFF002145, 0x00FFFFFF, 0x94FF28FF };
	const Uint32 PixelsTo[] = {0x00FF0000, 0xFF00FFFF, 0xFFFFFFFF, 0x9772FF13, 0xFF002145, 0x00FFFFFF };
	const int NumColors = sizeof(PixelsFrom)/sizeof(*PixelsFrom);

	//Set up an image of a million pixels in size. Set all values to values
	//in the 'pixels from' range.
	std::vector<Uint32> image(1000000);
	for(int n = 0; n != image.size(); ++n) {
		image[n] = PixelsFrom[n%NumColors];
	}

	//set up our table mapping pixels from -> pixels to.
	typedef std::map<Uint32, Uint32> PixelTable;
	PixelTable table;
	for(int n = 0; n != NumColors; ++n) {
		table.insert(std::pair<Uint32, Uint32>(PixelsFrom[n], PixelsTo[n]));
	}

	//now go over the image and map all source pixels to their destinations.
	//this is the part that we want to benchmark.
	BENCHMARK_LOOP {
		for(int n = 0; n != image.size(); ++n) {
			image[n] = table[image[n]];
		}
	}
}
