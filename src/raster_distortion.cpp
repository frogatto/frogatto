#include <math.h>
#include <stdlib.h>

#if defined(TARGET_OS_HARMATTAN) || defined(TARGET_PANDORA) || defined(TARGET_TEGRA) || defined(TARGET_BLACKBERRY)
#include <GLES/gl.h>
#else
#include <GL/gl.h>
#endif

#include "raster_distortion.hpp"

namespace graphics {

raster_distortion::raster_distortion(const rect& r) : area_(r), cycle_(0)
{}

raster_distortion::~raster_distortion()
{}

water_distortion::water_distortion(int offset, const rect& r) : raster_distortion(r), offset_(offset)
{}

void water_distortion::distort_point(GLfloat* x, GLfloat* y) const
{
	*x = *x + 8.0*sin((offset_ + *x)/GLfloat(20.0)) - 5.0*sin((offset_/4 + *x * 3)/GLfloat(20.0));
}

int water_distortion::granularity_x() const
{
	return 20;
}

int water_distortion::granularity_y() const
{
	return 10000;
}

variant water_distortion::get_value(const std::string& key) const
{
	return variant();
}

radial_distortion::radial_distortion(int x, int y, int radius, int intensity)
  : raster_distortion(rect(x - radius, y - radius, radius*2, radius*2)),
    x_(x), y_(y), radius_(radius), intensity_(intensity)
{
}

void radial_distortion::distort_point(GLfloat* x, GLfloat* y) const
{
	if(*x == x_ && *y == y_) {
		return;
	}

	const GLfloat vector_x = *x - x_;
	const GLfloat vector_y = *y - y_;
	const GLfloat distance = sqrt(vector_x*vector_x + vector_y*vector_y);
	if(distance > radius_) {
		return;
	}

	const GLfloat unit_vector_x = vector_x/distance;
	const GLfloat unit_vector_y = vector_y/distance;

	const GLfloat distort = sin(distance + cycle()*0.2)*intensity_*((radius_ - distance)/radius_);
	*x += unit_vector_x*distort;
	*y += unit_vector_y*distort;
}

int radial_distortion::granularity_x() const
{
	return 10;
}

int radial_distortion::granularity_y() const
{
	return 10;
}

variant radial_distortion::get_value(const std::string& key) const
{
	if(key == "radius") {
		return variant(radius_*1000);
	} else {
		return variant();
	}
}

void radial_distortion::set_value(const std::string& key, const variant& value)
{
	if(key == "radius") {
		radius_ = value.as_int()/1000.0;
		std::cerr << "set radius: " << radius_ << "\n";
		set_area(rect(x_ - radius_, y_ - radius_, radius_*2, radius_*2));
	}
}

}
