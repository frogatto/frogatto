#ifndef GEOMETRY_HPP_INCLUDED
#define GEOMETRY_HPP_INCLUDED

#include "SDL.h"

#include <iostream>
#include <string>
#include <vector>

struct point {
	explicit point(const std::string& str);
	explicit point(int x=0, int y=0) : x(x), y(y)
	{}

	std::string to_string() const;

	int x, y;
};

bool operator==(const point& a, const point& b);
bool operator!=(const point& a, const point& b);
bool operator<(const point& a, const point& b);

class rect {
public:
	static rect from_coordinates(int x1, int y1, int x2, int y2);
	explicit rect(const std::string& str);
	explicit rect(int x=0, int y=0, int w=0, int h=0);
	int x() const;
	int y() const;
	int x2() const;
	int y2() const;
	int w() const;
	int h() const;

	int mid_x() const { return (x() + x2())/2; }
	int mid_y() const { return (y() + y2())/2; }

	std::string to_string() const;

	SDL_Rect sdl_rect() const;

	bool empty() const { return w() == 0 || h() == 0; }
private:
	point top_left_, bottom_right_;
};

bool point_in_rect(const point& p, const rect& r);
bool rects_intersect(const rect& a, const rect& b);
rect intersection_rect(const rect& a, const rect& b);
void rect_difference(const rect& a, const rect& b, std::vector<rect>* output); //returns a vector containing the parts of A that don't intersect B

inline bool operator==(const rect& a, const rect& b) {
	return a.x() == b.x() && a.y() == b.y() && a.w() == b.w() && a.h() == b.h();
}

inline bool operator!=(const rect& a, const rect& b) {
	return !operator==(a, b);
}

std::ostream& operator<<(std::ostream& s, const rect& r);

#endif
