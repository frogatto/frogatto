#ifndef BACKGROUND_HPP_INCLUDED
#define BACKGROUND_HPP_INCLUDED

#include <string>
#include <vector>

#include "boost/shared_ptr.hpp"

#include "SDL.h"

#include "texture.hpp"
#include "wml_node_fwd.hpp"

class level;

class background
{
public:
	static boost::shared_ptr<background> get(const std::string& id);

	explicit background(const wml::const_node_ptr& node);
	const std::string& id() const { return id_; }
	wml::node_ptr write() const;
	void draw(double x, double y, int rotation) const;
	void draw_foreground(double x, double y, int rotation) const;
private:
	std::string id_;
	SDL_Color top_, bot_;
	int width_, height_;

	struct layer {
		std::string image;
		std::string image_formula;
		mutable graphics::texture texture;
		int xscale, yscale;
		int scale;
		int yoffset;
		GLfloat color[4];

		// Top and bottom edges of the background.
		mutable int y1, y2;

		//if true, this layer is actually drawn in the foreground.
		bool foreground;
	};

	void draw_layer(int x, int y, int rotation, const layer& bg) const;

	std::vector<layer> layers_;
};

#endif
