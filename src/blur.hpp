#ifndef BLUR_HPP_INCLUDED
#define BLUR_HPP_INCLUDED

#include <deque>

class frame;

class blur_info
{
public:
	blur_info(double alpha, double fade, int granularity);
	void copy_settings(const blur_info& info);

	void next_frame(int start_x, int start_y, int end_x, int end_y,
	                const frame* f, int time_in_frame, bool facing,
					bool upside_down, float rotate);

	void draw() const;

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
