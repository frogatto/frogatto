#include <math.h>
#include <stdlib.h>

#include "GL/gl.h"

#include "raster_distortion.hpp"

namespace graphics {

raster_distortion::raster_distortion(const rect& r) : area_(r)
{}

raster_distortion::~raster_distortion()
{}

water_distortion::water_distortion(int offset, const rect& r) : raster_distortion(r), offset_(offset)
{}

void water_distortion::distort_point(int* x, int* y) const
{
	*x = *x + 8.0*sin((offset_ + *x)/GLfloat(20.0));
}

int water_distortion::granularity_x() const
{
	return 20;
}

int water_distortion::granularity_y() const
{
	return 10000;
}

}
