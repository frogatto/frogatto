#include <vector>

#include "formatter.hpp"
#include "geometry.hpp"
#include "string_utils.hpp"

point::point(const std::string& str)
{
	std::vector<std::string> items = util::split(str);
	if(items.size() == 2) {
		x = atoi(items[0].c_str());
		y = atoi(items[1].c_str());
	} else {
		x = y = 0;
	}
}

std::string point::to_string() const
{
	return formatter() << x << "," << y;
}

std::string rect::to_string() const
{
	return formatter() << x() << "," << y() << "," << x2() << "," << y2();
}

SDL_Rect rect::sdl_rect() const
{
	SDL_Rect r = {x(), y(), w(), h()};
	return r;
}

rect rect::from_coordinates(int x1, int y1, int x2, int y2)
{
	if(x1 > x2) {
		std::swap(x1, x2);
	}

	if(y1 > y2) {
		std::swap(y1, y2);
	}

	return rect(x1, y1, (x2 - x1) + 1, (y2 - y1) + 1);
}

rect::rect(const std::string& str)
{
	if(str.empty()) {
		*this = rect();
		return;
	}

	std::vector<std::string> items = util::split(str);
	if(items.size() == 3) {
		*this = rect::from_coordinates(
		    atoi(items[0].c_str()), atoi(items[1].c_str()),
		    atoi(items[2].c_str()), 1);
	} else if(items.size() == 4) {
		*this = rect::from_coordinates(
		    atoi(items[0].c_str()), atoi(items[1].c_str()),
		    atoi(items[2].c_str()), atoi(items[3].c_str()));
	} else {
		*this = rect();
	}
}

rect::rect(int x, int y, int w, int h)
  : top_left_(std::min(x, x+w), std::min(y, y+h)),
    bottom_right_(std::max(x, x+w), std::max(y, y+h))
{
}

int rect::x() const
{
	return top_left_.x;
}

int rect::y() const
{
	return top_left_.y;
}

int rect::x2() const
{
	return bottom_right_.x;
}

int rect::y2() const
{
	return bottom_right_.y;
}

int rect::w() const
{
	return bottom_right_.x - top_left_.x;
}

int rect::h() const
{
	return bottom_right_.y - top_left_.y;
}

bool point_in_rect(const point& p, const rect& r)
{
	return p.x >= r.x() && p.y >= r.y() && p.x < r.x2() && p.y < r.y2();
}

bool rects_intersect(const rect& a, const rect& b)
{
	if(a.x2() < b.x() || b.x2() < a.x()) {
		return false;
	}

	if(a.y2() < b.y() || b.y2() < a.y()) {
		return false;
	}

	if(a.w() == 0 || a.h() == 0 || b.w() == 0 || b.h() == 0) {
		return false;
	}
	
	return true;
}

rect intersection_rect(const rect& a, const rect& b)
{
	const int x = std::max(a.x(), b.x());
	const int y = std::max(a.y(), b.y());
	const int w = std::min(a.x2(), b.x2()) - x;
	const int h = std::min(a.y2(), b.y2()) - y;
	return rect(x, y, w, h);
}

std::ostream& operator<<(std::ostream& s, const rect& r)
{
	s << "rect(" << r.x() << ", " << r.y() << ", " << r.x2() << ", " << r.y2() << ")";
	return s;
}
