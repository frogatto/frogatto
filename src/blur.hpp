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
#ifndef BLUR_HPP_INCLUDED
#define BLUR_HPP_INCLUDED

#include <deque>

class frame;

//class which represents the blur information for a single object.
//a blur contains three parameters:
// - alpha: the initial alpha value of the blurred vision of the object.
// - fade: the rate at which the alpha fades each frame
// - granularity: the number of copies of the object that are made
//                every cycle.
class blur_info
{
public:
	blur_info(double alpha, double fade, int granularity);

	//function to copy settings into another blur_info instance. This will
	//keep our blur_frames as they are, but copy in the alpha/fade/granularity
	//settings and so change our blur behavior from then on.
	void copy_settings(const blur_info& info);

	//function to progress to the next frame. We are given starting and
	//ending position of the object, along with its drawing settings.
	//
	//'granularity' copies of the object's image will be made, linearly
	//interpolated between start_x,start_y and end_x,end_y.
	void next_frame(int start_x, int start_y, int end_x, int end_y,
	                const frame* f, int time_in_frame, bool facing,
					bool upside_down, float start_rotate, float rotate);

	void draw() const;

	//returns true iff our granularity is now 0 and we have no blur_frames.
	bool destroyed() const;

private:
	struct blur_frame {
		const frame* object_frame;
		int time_in_frame;
		double x, y;
		bool facing, upside_down;
		float rotate;
		double fade;
	};

	double alpha_;
	double fade_;
	int granularity_;
	std::deque<blur_frame> frames_;
};

#endif
