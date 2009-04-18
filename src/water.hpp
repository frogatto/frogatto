#ifndef WATER_HPP_INCLUDED
#define WATER_HPP_INCLUDED

#include <vector>

#include "formula_fwd.hpp"
#include "geometry.hpp"
#include "raster_distortion.hpp"
#include "wml_node_fwd.hpp"

class entity;
class level;

class water
{
public:
	explicit water(wml::const_node_ptr node);

	wml::node_ptr write() const;
	void begin_drawing();
	void end_drawing();
	void set_surface_detection_rects(int zorder);
	bool draw(int begin_layer, int end_layer, int x, int y, int w, int h) const;
	void process(const level& lvl);

	int min_zorder() const;
	int max_zorder() const;
	int min_offset() const;
	int max_offset() const;

	void get_current(const entity& e, int* velocity_x, int* velocity_y) const;
	
private:

	struct area {
		explicit area(const rect& r) : rect_(r), distortion_(0, rect(0,0,0,0))
		{}
		rect rect_;
		graphics::water_distortion distortion_;
		std::vector<char> draw_detection_buf_;
	};

	std::vector<area> areas_;

	bool draw_area(const area& a, int begin_layer, int end_layer, int x, int y, int w, int h) const;

	struct zorder_pos {
		int zorder;
		int offset;
		SDL_Color color;
	};

	std::vector<zorder_pos> positions_;

	enum { BadOffset = -100000 };
	int get_offset(int zorder) const;
	SDL_Color get_color(int offset) const;

	game_logic::const_formula_ptr current_x_formula_, current_y_formula_;
};

#endif
