#include "graphics.hpp"
#include "blur.hpp"
#include "foreach.hpp"
#include "frame.hpp"

blur_info::blur_info(double alpha, double fade, int granularity)
  : alpha_(alpha), fade_(fade), granularity_(granularity)
{
}

void blur_info::copy_settings(const blur_info& o)
{
	alpha_ = o.alpha_;
	fade_ = o.fade_;
	granularity_ = o.granularity_;
}

void blur_info::next_frame(int start_x, int start_y, int end_x, int end_y,
                const frame* object_frame, int time_in_frame, bool facing,
				bool upside_down, float start_rotate, float rotate) {
	foreach(blur_frame& f, frames_) {
		f.fade -= fade_;
	}

	while(!frames_.empty() && frames_.front().fade <= 0.0) {
		frames_.pop_front();
	}

	for(int n = 0; n < granularity_; ++n) {
		blur_frame f;
		f.object_frame = object_frame;
		f.x = (start_x*n + end_x*(granularity_ - n))/granularity_;
		f.y = (start_y*n + end_y*(granularity_ - n))/granularity_;
		f.time_in_frame = time_in_frame;
		f.facing = facing;
		f.upside_down = upside_down;
		f.rotate = (start_rotate*n + rotate*(granularity_ - n))/granularity_;
		f.fade = alpha_ + (fade_*(granularity_ - n))/granularity_;
		frames_.push_back(f);
	}
}

void blur_info::draw() const
{
	GLfloat color[4];
#if defined(USE_GLES2) && defined(GL_ES_VERSION_2_0)
	glGetFloatv_1(GL_CURRENT_COLOR, color);
#else
	glGetFloatv(GL_CURRENT_COLOR, color);
#endif
	foreach(const blur_frame& f, frames_) {
		glColor4f(color[0], color[1], color[2], color[3]*f.fade);
		f.object_frame->draw(f.x, f.y, f.facing, f.upside_down, f.time_in_frame, f.rotate);
	}

	glColor4f(color[0], color[1], color[2], color[3]);
}

bool blur_info::destroyed() const
{
	return granularity_ == 0 && frames_.empty();
}
