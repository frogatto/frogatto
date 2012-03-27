#ifndef GEOMETRY_HPP_INCLUDED
#define GEOMETRY_HPP_INCLUDED

#include "graphics.hpp"
#include "variant.hpp"

#include <iostream>
#include <string>
#include <vector>

struct point {
	explicit point(const variant& v);
	explicit point(const std::string& str);
	explicit point(int x=0, int y=0) : x(x), y(y)
	{}

	explicit point(const std::vector<int>& v);

	variant write() const;
	std::string to_string() const;

	union {
		struct { int x, y; };
		int buf[2];
	};
};

bool operator==(const point& a, const point& b);
bool operator!=(const point& a, const point& b);
bool operator<(const point& a, const point& b);

namespace game_logic {
class formula_callable;
}

class rect {
public:
	static rect from_coordinates(int x1, int y1, int x2, int y2);
	explicit rect(const std::string& str);
	explicit rect(int x=0, int y=0, int w=0, int h=0);
	explicit rect(const std::vector<int>& v);
	explicit rect(const variant& v);
	int x() const;
	int y() const;
	int x2() const;
	int y2() const;
	int w() const;
	int h() const;

	int mid_x() const { return (x() + x2())/2; }
	int mid_y() const { return (y() + y2())/2; }

	const point& top_left() const { return top_left_; }
	const point& bottom_right() const { return bottom_right_; }

	std::string to_string() const;
	variant write() const;

	SDL_Rect sdl_rect() const;

	bool empty() const { return w() == 0 || h() == 0; }

	game_logic::formula_callable* callable() const;
private:
	point top_left_, bottom_right_;
};

bool point_in_rect(const point& p, const rect& r);
bool rects_intersect(const rect& a, const rect& b);
rect intersection_rect(const rect& a, const rect& b);
int rect_difference(const rect& a, const rect& b, rect* output); //returns a vector containing the parts of A that don't intersect B

rect rect_union(const rect& a, const rect& b);

inline bool operator==(const rect& a, const rect& b) {
	return a.x() == b.x() && a.y() == b.y() && a.w() == b.w() && a.h() == b.h();
}

inline bool operator!=(const rect& a, const rect& b) {
	return !operator==(a, b);
}

std::ostream& operator<<(std::ostream& s, const rect& r);

#endif
