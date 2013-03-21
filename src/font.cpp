/*
	Copyright (C) 2003-2013 by David White <davewx7@gmail.com>
	
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <iostream>
#include <map>

#include <boost/tuple/tuple.hpp>

#include "asserts.hpp"
#include "color_utils.hpp"
#include "font.hpp"
#include "foreach.hpp"
#include "formatter.hpp"
#include "module.hpp"
#include "string_utils.hpp"
#include "surface.hpp"

/*  This manages the TTF loading library, and allows you to use fonts.
	The only thing one will normally need to use is render_text(), and 
	possibly char_width(), char_height() if you need to know the size 
	of the resulting text. */

namespace font {
#if TARGET_IPHONE_SIMULATOR || TARGET_OS_HARMATTAN || TARGET_OS_IPHONE
class TTF_Font;
#endif

namespace {
const char* FontFile = "UbuntuMono-R";

typedef std::map<std::pair<std::string, int>, TTF_Font*> font_map;
font_map font_table;

std::map<std::string,std::string>& get_font_list()
{
	static std::map<std::string,std::string> res;
	return res;
}

const std::string& get_font_path(const std::string& name) 
{
	std::map<std::string,std::string>& res = get_font_list();
	if(res.empty()) {
		module::get_unique_filenames_under_dir("data/fonts/", &res);
	}
	std::map<std::string, std::string>::const_iterator itor = module::find(res, name);
	if(itor == res.end()) {
		ASSERT_LOG(false, "FONT FILE NOT FOUND: " << name);
	}
	return itor->second;
}

TTF_Font* get_font(int size, const std::string& font_name="")
{
#if !TARGET_IPHONE_SIMULATOR && !TARGET_OS_HARMATTAN && !TARGET_OS_IPHONE
	std::string fontn = get_font_path((font_name.empty() ? FontFile : font_name) + ".ttf");
	TTF_Font* font = NULL;
	font_map::const_iterator it = font_table.find(std::pair<std::string,int>(fontn,size));
	if(it == font_table.end()) {
		font = TTF_OpenFont(fontn.c_str(), size);
		if(font == NULL) {
			std::ostringstream s;

			s << "Failed to open font: " << fontn;
			ASSERT_LOG(false, s.str());
			exit(0);
		}
		font_table[std::pair<std::string,int>(fontn,size)] = font;
	} else {
		font = it->second;
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
	std::string font_name;

	bool operator<(const CacheKey& k) const {
		return text < k.text || text == k.text && color < k.color 
			|| text == k.text && color == k.color && font_size < k.font_size 
			|| text == k.text && color == k.color && font_size == k.font_size && font_name < k.font_name;
	}
};

typedef std::map<CacheKey, graphics::texture> RenderCache;

int g_render_cache_size = 0;

RenderCache& cache() {
	static RenderCache instance;
	return instance;
}

bool fonts_initialized = false;

}

bool is_init() {
	return fonts_initialized;
}

manager::manager()
{
#if !TARGET_IPHONE_SIMULATOR && !TARGET_OS_HARMATTAN && !TARGET_OS_IPHONE
	const int res = TTF_Init();
	if(res == -1) {
		std::cerr << "Could not initialize ttf\n";
		exit(0);
	} else {
		fonts_initialized = true;
		std::cerr << "initialized ttf\n";
	}
#endif
}

manager::~manager()
{
#if !TARGET_IPHONE_SIMULATOR && !TARGET_OS_HARMATTAN && !TARGET_OS_IPHONE
	font_map::iterator it = font_table.begin();
	while(it != font_table.end()) {
		TTF_CloseFont(it->second);
		font_table.erase(it++);
	}

	TTF_Quit();
#endif
}

graphics::texture render_text_uncached(const std::string& text,
                                       const SDL_Color& color, int size, const std::string& font_name)
{
#if !TARGET_IPHONE_SIMULATOR && !TARGET_OS_HARMATTAN && !TARGET_OS_IPHONE
	TTF_Font* font = get_font(size, font_name);

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
#if SDL_VERSION_ATLEAST(2, 0, 0)
		s = graphics::surface(SDL_CreateRGBSurface(0, width, height, f->BitsPerPixel, f->Rmask, f->Gmask, f->Bmask, f->Amask));
#else
		s = graphics::surface(SDL_CreateRGBSurface(SDL_SWSURFACE, width, height, f->BitsPerPixel, f->Rmask, f->Gmask, f->Bmask, f->Amask));
#endif
		int ypos = 0;
		foreach(graphics::surface part, parts) {
			SDL_Rect rect = {0, ypos, part->w, part->h};
#if SDL_VERSION_ATLEAST(2, 0, 0)
			SDL_SetSurfaceBlendMode(part.get(), SDL_BLENDMODE_NONE);
#else
			SDL_SetAlpha(part.get(), 0, SDL_ALPHA_OPAQUE);
#endif
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
                              const SDL_Color& color, int size, const std::string& font_name)
{
	CacheKey key = {text, graphics::color(color.r, color.g, color.b), size, font_name};
	RenderCache::const_iterator cache_itor = cache().find(key);
	if(cache_itor != cache().end()) {
		return cache_itor->second;
	}

	graphics::texture res = render_text_uncached(text, color, size, font_name);

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

int char_width(int size, const std::string& fn)
{
	static std::map<int, int> size_cache;
	int& width = size_cache[size];
	if(width) {
		return width;
	}
	SDL_Color color = {0, 0, 0, 0};
	graphics::texture t(render_text("ABCDEFABCDEF", color, size, fn));
	width = t.width()/12;
	return width;
}

int char_height(int size, const std::string& fn)
{
	static std::map<int, int> size_cache;
	int& height = size_cache[size];
	if(height) {
		return height;
	}
	SDL_Color color = {0, 0, 0, 0};
	graphics::texture t(render_text("A", color, size, fn));
	height = t.height();
	return height;
}

void reload_font_paths()
{
	get_font_list().clear();
}

}
