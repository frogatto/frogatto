#include <iostream>
#include <map>

#include <boost/tuple/tuple.hpp>

#include "color_utils.hpp"
#include "font.hpp"
#include "foreach.hpp"
#include "string_utils.hpp"
#include "surface.hpp"

/*  This manages the TTF loading library, and allows you to use fonts.
	The only thing one will normally need to use is render_text(), and possibly char_width(), char_height() if you need to know the size of the resulting text. */

namespace font {
#if TARGET_IPHONE_SIMULATOR || TARGET_OS_HARMATTAN || TARGET_OS_IPHONE
class TTF_Font;
#endif

namespace {
const char* FontFile = "UbuntuMono-R.ttf";

std::map<int, TTF_Font*> font_table;

TTF_Font* get_font(int size)
{
#if !TARGET_IPHONE_SIMULATOR && !TARGET_OS_HARMATTAN && !TARGET_OS_IPHONE
	TTF_Font*& font = font_table[size];
	if(font == NULL) {
		font = TTF_OpenFont(FontFile, size);
		if(font == NULL) {
			std::cerr << "Failed to open font: " << FontFile << "\n";
			exit(0);
		}
	}

	return font;
#else
	return NULL;
#endif
}

struct CacheKey {
	std::string text;
	graphics::color color;
	int font_size;

	bool operator<(const CacheKey& k) const {
		return text < k.text || text == k.text && color < k.color ||
		       text == k.text && color == k.color && font_size < k.font_size;
	}
};

typedef std::map<CacheKey, graphics::texture> RenderCache;

int g_render_cache_size = 0;

RenderCache& cache() {
	static RenderCache instance;
	return instance;
}

}

manager::manager()
{
#if !TARGET_IPHONE_SIMULATOR && !TARGET_OS_HARMATTAN && !TARGET_OS_IPHONE
	const int res = TTF_Init();
	if(res == -1) {
		std::cerr << "Could not initialize ttf\n";
		exit(0);
	} else {
		std::cerr << "initialized ttf\n";
	}
#endif
}

manager::~manager()
{
#if !TARGET_IPHONE_SIMULATOR && !TARGET_OS_HARMATTAN && !TARGET_OS_IPHONE
	TTF_Quit();
#endif
}

graphics::texture render_text_uncached(const std::string& text,
                                       const SDL_Color& color, int size)
{
#if !TARGET_IPHONE_SIMULATOR && !TARGET_OS_HARMATTAN && !TARGET_OS_IPHONE
	TTF_Font* font = get_font(size);

	graphics::surface s;
	if(std::find(text.begin(), text.end(), '\n') == text.end()) {
		s = graphics::surface(TTF_RenderUTF8_Blended(font, text.c_str(), color));
	} else {
		std::vector<graphics::surface> parts;
		std::vector<std::string> lines = util::split(text, '\n');
		int height = 0, width = 0;
		foreach(const std::string& line, lines) {
			parts.push_back(graphics::surface(TTF_RenderUTF8_Blended(font, line.c_str(), color)));
			if(parts.back().get() == NULL) {
				std::cerr << "FAILED TO RENDER STRING: '" << line << "'\n";
				throw error();
			}

			if(parts.back()->w > width) {
				width = parts.back()->w;
			}

			height += parts.back()->h;
		}

		const SDL_PixelFormat* f = parts.front()->format;
		s = graphics::surface(SDL_CreateRGBSurface(SDL_SWSURFACE, width, height, f->BitsPerPixel, f->Rmask, f->Gmask, f->Bmask, f->Amask));
		int ypos = 0;
		foreach(graphics::surface part, parts) {
			SDL_Rect rect = {0, ypos, part->w, part->h};
			SDL_SetAlpha(part.get(), 0, SDL_ALPHA_OPAQUE);
			SDL_BlitSurface(part.get(), NULL, s.get(), &rect);
			ypos += part->h;
		}
	}
#else
	graphics::surface s;
#endif
	return graphics::texture::get_no_cache(s);
}

graphics::texture render_text(const std::string& text,
                              const SDL_Color& color, int size)
{
	CacheKey key = {text, graphics::color(color.r, color.g, color.b), size};
	RenderCache::const_iterator cache_itor = cache().find(key);
	if(cache_itor != cache().end()) {
		return cache_itor->second;
	}

	graphics::texture res = render_text_uncached(text, color, size);

	if(res.width()*res.height() <= 256*256) {
		if(cache().size() > 512) {
			cache().clear();
			g_render_cache_size = 0;
		}
		cache()[key] = res;
		g_render_cache_size += res.width()*res.height()*4;
	}
	return res;
}

int char_width(int size)
{
	static std::map<int, int> size_cache;
	int& width = size_cache[size];
	if(width) {
		return width;
	}
	SDL_Color color = {0, 0, 0, 0};
	graphics::texture t(render_text("ABCDEFABCDEF", color, size));
	width = t.width()/12;
	return width;
}

int char_height(int size)
{
	static std::map<int, int> size_cache;
	int& height = size_cache[size];
	if(height) {
		return height;
	}
	SDL_Color color = {0, 0, 0, 0};
	graphics::texture t(render_text("A", color, size));
	height = t.height();
	return height;
}

}
