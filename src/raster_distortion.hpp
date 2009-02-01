#ifndef RASTER_DISTORTION_HPP_INCLUDED
#define RASTER_DISTORTION_HPP_INCLUDED

#include "geometry.hpp"

namespace graphics {

//Class which represents distortions which affect blitting operations.
//This is useful to generate 'waves' such as for water, heat, etc.
class raster_distortion
{
public:
	explicit raster_distortion(const rect& r);
	virtual ~raster_distortion();

	//function which map undistorted co-ordinates into their distorted equivalents.
	virtual void distort_point(int* x, int* y) const = 0;
	
	//functions which determine the granularity of the distortion on each axis.
	//This represents the size of the edges of the rectangles that textures will
	//be divided into. The lower the value, the finer the granularity, and the
	//more expensive the operations.
	virtual int granularity_x() const = 0;
	virtual int granularity_y() const = 0;

	//the area that the raster distortion takes effect in.
	const rect& area() const { return area_; }
private:
	rect area_;
};

class water_distortion : public raster_distortion
{
public:
	explicit water_distortion(int offset, const rect& r);

	void distort_point(int* x, int* y) const;

	int granularity_x() const;
	int granularity_y() const;
private:
	int offset_;
};

}

#endif
