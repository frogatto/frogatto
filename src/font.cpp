#include <iostream>
#include <map>

#include "font.hpp"
#include "surface.hpp"

/*  This manages the TTF loading library, and allows you to use fonts.
	The only thing one will normally need to use is render_text(), and possibly char_width(), char_height() if you need to know the size of the resulting text. */

namespace font {

namespace {
const char* FontFile = "FreeMono.ttf";

std::map<int, TTF_Font*> font_table;

TTF_Font* get_font(int size)
{
#if !TARGET_IPHONE_SIMULATOR && !TARGET_OS_IPHONE
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

struct cache_entry
{
	std::string text;
	SDL_Color color;
	int size;
	graphics::texture texture;
};

const int CacheSize = 4;
cache_entry cache[CacheSize];
int cache_index = 0;

}

manager::manager()
{
#if !TARGET_IPHONE_SIMULATOR && !TARGET_OS_IPHONE
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
#if !TARGET_IPHONE_SIMULATOR && !TARGET_OS_IPHONE
	TTF_Quit();
#endif
}

graphics::texture render_text(const std::string& text,
                              const SDL_Color& color, int size)
{
	for(int n = 0; n != CacheSize; ++n) {
		if(text == cache[n].text && memcmp(&color, &cache[n].color, sizeof(color)) == 0 && size == cache[n].size) {
			return cache[n].texture;
		}
	}
#if !TARGET_IPHONE_SIMULATOR && !TARGET_OS_IPHONE
	TTF_Font* font = get_font(size);
	graphics::surface s(TTF_RenderUTF8_Blended(font, text.c_str(), color));
#else
	graphics::surface s;
#endif
	graphics::texture res = graphics::texture::get_no_cache(s);
	cache_index = (cache_index+1)%CacheSize;
	cache[cache_index].text = text;
	cache[cache_index].color = color;
	cache[cache_index].size = size;
	cache[cache_index].texture = res;
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
