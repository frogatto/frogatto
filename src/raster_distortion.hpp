/*
	Copyright (C) 2003-2013 by David White <davewx7@gmail.com>
	
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef RASTER_DISTORTION_HPP_INCLUDED
#define RASTER_DISTORTION_HPP_INCLUDED

#include "graphics.hpp"
#include "formula_callable.hpp"
#include "geometry.hpp"

namespace graphics {

//Class which represents distortions which affect blitting operations.
//This is useful to generate 'waves' such as for water, heat, etc.
class raster_distortion : public game_logic::formula_callable
{
public:
	explicit raster_distortion(const rect& r);
	virtual ~raster_distortion();

	//function which map undistorted co-ordinates into their distorted equivalents.
	virtual void distort_point(GLfloat* x, GLfloat* y) const = 0;
	
	//functions which determine the granularity of the distortion on each axis.
	//This represents the size of the edges of the rectangles that textures will
	//be divided into. The lower the value, the finer the granularity, and the
	//more expensive the operations.
	virtual int granularity_x() const = 0;
	virtual int granularity_y() const = 0;

	//the area that the raster distortion takes effect in.
	rect area() const { return area_; }
	void set_area(const rect& area) { area_ = area; }

	int cycle() const { return cycle_; }
	void next_cycle() const { ++cycle_; }
	void set_cycle(int n) { cycle_ = n; }
private:
	virtual variant get_value(const std::string& key) const { return variant(); }
	rect area_;
	mutable int cycle_;
};

typedef boost::intrusive_ptr<raster_distortion> raster_distortion_ptr;
typedef boost::intrusive_ptr<const raster_distortion> const_raster_distortion_ptr;

class water_distortion : public raster_distortion
{
public:
	water_distortion(int offset, const rect& r);

	void distort_point(GLfloat* x, GLfloat* y) const;

	int granularity_x() const;
	int granularity_y() const;
private:
	virtual variant get_value(const std::string& key) const;
	int offset_;
};

class radial_distortion : public raster_distortion
{
public:
	radial_distortion(int x, int y, int radius, int intensity=5);

	void distort_point(GLfloat* x, GLfloat* y) const;

	int granularity_x() const;
	int granularity_y() const;
private:
	int x_, y_;
	GLfloat radius_;
	GLfloat intensity_;

	virtual variant get_value(const std::string& key) const;
	virtual void set_value(const std::string& key, const variant& value);
};

}

#endif
