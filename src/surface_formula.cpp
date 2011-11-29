#include <iostream>
#include <map>
#include <SDL.h>
#ifndef SDL_VIDEO_OPENGL_ES
#include <GL/glew.h>
#endif

#include "asserts.hpp"
#include "concurrent_cache.hpp"
#include "filesystem.hpp"
#include "foreach.hpp"
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

cache_map& cache() {
	static cache_map instance;
	return instance;
}

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
	                  const std::vector<expression_ptr>& args,
					  const formula_callable_definition* callable_def) const;
private:
	surface surf_;
};

expression_ptr surface_formula_symbol_table::create_function(
                           const std::string& fn,
                           const std::vector<expression_ptr>& args,
						   const formula_callable_definition* callable_def) const
{
	if(fn == "rgba") {
		return expression_ptr(new rgba_function(surf_, args));
	} else {
		return function_symbol_table::create_function(fn, args, callable_def);
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

	bool is_alpha() const { return a == 0x00; }
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
	surface surf = cache().get(key);
	if(surf.get() == NULL) {
		surf = input.clone();
		run_formula(surf, algo);
		cache().put(key, surf);
	}

	return surf;
}

namespace {
typedef std::map<std::pair<std::string, GLuint>, GLuint> shader_object_map;
shader_object_map shader_object_cache;

typedef std::map<std::pair<std::vector<std::string>,std::vector<std::string> >, GLuint> shader_map;
shader_map shader_cache;

void check_shader_errors(const std::string& fname, GLuint shader)
{
#ifndef SDL_VIDEO_OPENGL_ES
	GLint value = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &value);
	if(value == GL_FALSE) {
		char buf[1024*16];
		GLint len;
		glGetShaderInfoLog(shader, sizeof(buf), &len, buf);
		std::string errors(buf, buf + len);
		ASSERT_LOG(false, "COMPILE ERROR IN SHADER " << fname << ": " << errors);
	}
#endif
}

GLuint compile_shader(const std::string& shader_file, GLuint type)
{
	GLuint& id = shader_object_cache[std::make_pair(shader_file, type)];
	if(id) {
		return id;
	}
#ifndef SDL_VIDEO_OPENGL_ES
	id = glCreateShader(type);

	const std::string file_data = sys::read_file("data/shaders/" + shader_file);

	const char* file_str = file_data.c_str();
	glShaderSource(id, 1, &file_str, NULL);

	glCompileShader(id);
	check_shader_errors(shader_file, id);

	return id;
#else
	return 0;
#endif
}

}

GLuint get_gl_shader(const std::vector<std::string>& vertex_shader_file,
                     const std::vector<std::string>& fragment_shader_file)
{
#ifndef SDL_VIDEO_OPENGL_ES
	if(vertex_shader_file.empty() || fragment_shader_file.empty()) {
		return 0;
	}

	shader_map::iterator itor = shader_cache.find(std::make_pair(vertex_shader_file, fragment_shader_file));
	if(itor != shader_cache.end()) {
		return itor->second;
	}

	std::vector<GLuint> shader_objects;
	foreach(const std::string& shader_file, vertex_shader_file) {
		shader_objects.push_back(compile_shader(shader_file, GL_VERTEX_SHADER));
	}

	foreach(const std::string& shader_file, fragment_shader_file) {
		shader_objects.push_back(compile_shader(shader_file, GL_FRAGMENT_SHADER));
	}

	GLuint program_id = glCreateProgram();

	foreach(GLuint shader_id, shader_objects) {
		glAttachShader(program_id, shader_id);
	}

	glLinkProgram(program_id);

	GLint link_status = 0;
	glGetProgramiv(program_id, GL_LINK_STATUS, &link_status);
	if(link_status != GL_TRUE) {
		char buf[1024*16];
		GLint len;
		glGetProgramInfoLog(program_id, sizeof(buf), &len, buf);
		std::string errors(buf, buf + len);
		ASSERT_LOG(false, "LINK ERROR IN SHADER PROGRAM: " << errors);
	}

	shader_cache[std::make_pair(vertex_shader_file, fragment_shader_file)] = program_id;

	glUseProgram(0);

	return program_id;
#else
	return 0;
#endif
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
