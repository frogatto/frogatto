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
	water();
	explicit water(wml::const_node_ptr node);

	wml::node_ptr write() const;

	void add_rect(const rect& r);
	void delete_rect(const rect& r);

	bool draw(int x, int y, int w, int h) const;
	int zorder() const { return zorder_; }

	void begin_drawing();
	void end_drawing() const;
	void process(const level& lvl);

	void get_current(const entity& e, int* velocity_x, int* velocity_y) const;

	bool is_underwater(const rect& r, rect* water_area=NULL) const;

	void add_wave(const point& p, double xvelocity, double height, double length, double delta_height, double delta_length);

	struct wave {
		double xpos;
		double xvelocity;
		double height;
		double length;
		double delta_height;
		double delta_length;

		void process();
	};
	
private:

	struct area {
		explicit area(const rect& r) : rect_(r), distortion_(0, rect(0,0,0,0))
		{}
		rect rect_;
		graphics::water_distortion distortion_;
		std::vector<char> draw_detection_buf_;

		std::vector<wave> waves_;
	};

	std::vector<area> areas_;

	bool draw_area(const area& a, int x, int y, int w, int h) const;

	bool draw_area(const area& a, int begin_layer, int end_layer, int x, int y, int w, int h) const;

	int zorder_;
	

/*
	struct zorder_pos {
		int zorder;
		int offset;
		SDL_Color color;
	};

	std::vector<zorder_pos> positions_;
*/

	enum { BadOffset = -100000 };

	game_logic::const_formula_ptr current_x_formula_, current_y_formula_;
};

#endif
