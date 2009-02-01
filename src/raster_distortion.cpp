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

int water_distortion::map_x(int src_x) const
{
	return src_x + 8.0*sin((offset_ + src_x)/GLfloat(20.0));
}

int water_distortion::map_y(int src_y) const
{
	return src_y;
}

int water_distortion::granularity_x() const
{
	return 5;
}

int water_distortion::granularity_y() const
{
	return 10000;
}

}
