#ifndef WATER_HPP_INCLUDED
#define WATER_HPP_INCLUDED

#include <vector>

#include "formula_fwd.hpp"
#include "wml_node_fwd.hpp"

class level;

class water
{
public:
	explicit water(wml::const_node_ptr node);

	wml::node_ptr write() const;
	bool draw(int begin_layer, int end_layer, int x, int y, int w, int h, const char* solid_buf) const;
	void process(const level& lvl);

	int min_zorder() const;
	int max_zorder() const;
	int min_offset() const;
	int max_offset() const;

	int get_water_level(int zorder) const { return level_ + get_offset(zorder); }
	
private:

	int level_;
	int min_zorder_;
	int max_zorder_;
	
	struct zorder_pos {
		int zorder;
		int offset;
		SDL_Color color;
	};

	std::vector<zorder_pos> positions_;

	enum { BadOffset = -100000 };
	int get_offset(int zorder) const;
	SDL_Color get_color(int offset) const;

	game_logic::const_formula_ptr water_level_formula_;
};

#endif
