#ifndef WATER_HPP_INCLUDED
#define WATER_HPP_INCLUDED

#include <vector>

#include "entity_fwd.hpp"
#include "formula_fwd.hpp"
#include "geometry.hpp"
#include "raster_distortion.hpp"
#include "variant.hpp"

class level;

class water
{
public:
	water();
	explicit water(variant node);

	variant write() const;

	//color must point to an array of 4 bytes.
	void add_rect(const rect& r, const unsigned char* color, variant obj);
	void delete_rect(const rect& r);

	bool draw(int x, int y, int w, int h) const;
	int zorder() const { return zorder_; }

	void begin_drawing();
	void end_drawing() const;
	void process(const level& lvl);

	void get_current(const entity& e, int* velocity_x, int* velocity_y) const;

	bool is_underwater(const rect& r, rect* water_area=NULL, variant* obj=NULL) const;

	void add_wave(const point& p, double xvelocity, double height, double length, double delta_height, double delta_length);

	struct wave {
		double xpos;
		double xvelocity;
		double height;
		double length;
		double delta_height;
		double delta_length;

		int left_bound, right_bound;

		void process();
	};
	
private:

	struct area {
		area(const rect& r, const unsigned char* color, variant obj);
		rect rect_;
		graphics::water_distortion distortion_;
		std::vector<char> draw_detection_buf_;

		std::vector<wave> waves_;

		//segments of the surface without solid.
		std::vector<std::pair<int, int> > surface_segments_;
		bool surface_segments_init_;

		unsigned char color_[4];
		variant obj_;
	};

	std::vector<area> areas_;

	static void init_area_surface_segments(const level& lvl, area& a);

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
