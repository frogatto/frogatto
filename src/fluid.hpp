#ifndef FLUID_HPP_INCLUDED
#define FLUID_HPP_INCLUDED

#include "wml_node_fwd.hpp"

#include <vector>

class fluid_pixel;
class fluid_tile;
class level;

class fluid {
public:
	fluid(int width, int height);

	static const int MaxPerPixel = 2;
	int fluids_at(int x, int y) const;
	fluid_pixel* get_pixel(int x, int y);
	fluid_pixel* create_pixel(int x, int y, fluid_tile** t);

	void process(level& lvl);
	void draw(int x, int y, int w, int h);

	void read(wml::const_node_ptr node);
	wml::node_ptr write() const;
private:
	int width_, height_;
	std::vector<fluid_tile*> tiles_;
};

#endif
