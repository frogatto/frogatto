#include <vector>

#include <algorithm>
#include "formatter.hpp"
#include "formula_callable.hpp"
#include "geometry.hpp"
#include "string_utils.hpp"
#include "unit_test.hpp"

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

bool operator==(const point& a, const point& b) {
	return a.x == b.x && a.y == b.y;
}

bool operator!=(const point& a, const point& b) {
	return !operator==(a, b);
}

bool operator<(const point& a, const point& b) {
	return a.x < b.x || a.x == b.x && a.y < b.y;
}

std::string rect::to_string() const
{
	return formatter() << x() << "," << y() << "," << (x2()-1) << "," << (y2()-1);
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
	if(items.size() == 2) {
		*this = rect::from_coordinates(
		    atoi(items[0].c_str()), atoi(items[1].c_str()), 1, 1);
	} else if(items.size() == 3) {
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
	if(a.x2() <= b.x() || b.x2() <= a.x()) {
		return false;
	}

	if(a.y2() <= b.y() || b.y2() <= a.y()) {
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

int rect_difference(const rect& a, const rect& b, rect* output)
{
	if (rects_intersect(a,b) == false){  //return empty if there's no intersection
		return -1;
	}
		
	/* returning 4 rectangles in this orientation:
	 _________
	 | |___| |
	 | |   | |
	 | |___| |
	 |_|___|_|  */

	const rect* begin_output = output;

	if(a.x() < b.x()) {
		//get the left section of the source rectangle
		*output++ = rect(a.x(), a.y(), b.x() - a.x(), a.h());
	}

	if(a.x() + a.w() > b.x() + b.w()) {
		*output++ = rect(b.x() + b.w(), a.y(), (a.x() + a.w()) - (b.x() + b.w()), a.h());
	}

	if(a.y() < b.y()) {
		const int x1 = std::max(a.x(), b.x());
		const int x2 = std::min(a.x() + a.w(), b.x() + b.w());
		*output++ = rect(x1, a.y(), x2 - x1, b.y() - a.y());
	}

	if(a.y() + a.h() > b.y() + b.h()) {
		const int x1 = std::max(a.x(), b.x());
		const int x2 = std::min(a.x() + a.w(), b.x() + b.w());
		*output++ = rect(x1, b.y() + b.h(), x2 - x1, (a.y() + a.h()) - (b.y() + b.h()));
	}
	
	return output - begin_output;
}

std::ostream& operator<<(std::ostream& s, const rect& r)
{
	s << "rect(" << r.x() << ", " << r.y() << ", " << r.x2() << ", " << r.y2() << ")";
	return s;
}

class rect_callable : public game_logic::formula_callable
{
	rect rect_;
	variant get_value(const std::string& key) const {
		if(key == "x") {
			return variant(rect_.x());
		} else if(key == "y") {
			return variant(rect_.y());
		} else if(key == "x2") {
			return variant(rect_.x2());
		} else if(key == "y2") {
			return variant(rect_.y2());
		} else if(key == "w") {
			return variant(rect_.w());
		} else if(key == "h") {
			return variant(rect_.h());
		} else {
			return variant();
		}
	}
public:
	explicit rect_callable(const rect& r) : rect_(r)
	{}
};

game_logic::formula_callable* rect::callable() const
{
	return new rect_callable(*this);
}

UNIT_TEST(rect)
{
	rect r(10, 10, 10, 10);
	rect r2(r.to_string());
	CHECK_EQ(r, r2);

	r = rect(10, 10, 10, 0);
	CHECK_NE(true, point_in_rect(point(15, 9), r));
	CHECK_NE(true, point_in_rect(point(15, 10), r));
	CHECK_NE(true, point_in_rect(point(15, 11), r));
	CHECK_EQ(r.h(), 0);
}

UNIT_TEST(rect_difference)
{
	rect r(100, 100, 200, 400);
	rect buf[4];

	CHECK_EQ(rect_difference(r, rect(0, 0, 100, 100), buf), -1);

	CHECK_EQ(rect_difference(r, rect(0, 0, 200, 1000), buf), 1);
	CHECK_EQ(buf[0], rect(200, 100, 100, 400));

	CHECK_EQ(rect_difference(r, rect(0, 0, 1000, 1000), buf), 0);

	CHECK_EQ(rect_difference(r, rect(150, 150, 50, 50), buf), 4);
	CHECK_EQ(buf[0], rect(100, 100, 50, 400));
	CHECK_EQ(buf[1], rect(200, 100, 100, 400));
	CHECK_EQ(buf[2], rect(150, 100, 50, 50));

	CHECK_EQ(rect_difference(rect(0, 891, 800, 1491), rect(-32, 1344, 1120, 2432), buf), 1);
	CHECK_EQ(buf[0], rect(0, 891, 800, 453));
}
