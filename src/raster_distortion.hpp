#ifndef RASTER_DISTORTION_HPP_INCLUDED
#define RASTER_DISTORTION_HPP_INCLUDED

#include "geometry.hpp"

namespace graphics {

class raster_distortion
{
public:
	explicit raster_distortion(const rect& r);
	virtual ~raster_distortion();
	virtual int map_x(int src_x) const = 0;
	virtual int map_y(int src_y) const = 0;
	
	virtual int granularity_x() const = 0;
	virtual int granularity_y() const = 0;

	const rect& area() const { return area_; }
private:
	rect area_;
};

class water_distortion : public raster_distortion
{
public:
	explicit water_distortion(int offset, const rect& r);

	int map_x(int src_x) const;
	int map_y(int src_y) const;

	int granularity_x() const;
	int granularity_y() const;
private:
	int offset_;
};

}

#endif
