#ifndef BACKGROUND_HPP_INCLUDED
#define BACKGROUND_HPP_INCLUDED

#include <vector>

#include "boost/shared_ptr.hpp"

#include "SDL.h"

#include "texture.hpp"
#include "wml_node_fwd.hpp"

class level;

class background
{
public:
	explicit background(const wml::const_node_ptr& node);
	wml::node_ptr write() const;
	void draw(double x, double y) const;
private:
	SDL_Color top_, bot_;
	int width_, height_;

	struct layer {
		std::string image;
		std::string image_formula;
		mutable graphics::texture texture;
		int xscale, yscale;
		int scale;
		int yoffset;
	};

	void draw_layer(int x, int y, const layer& bg) const;

	std::vector<layer> layers_;
};

#endif
