#include <iostream>

#include <boost/lexical_cast.hpp>

#include "asserts.hpp"
#include "foreach.hpp"
#include "frame.hpp"
#include "raster.hpp"
#include "sound.hpp"
#include "string_utils.hpp"
#include "texture.hpp"
#include "wml_node.hpp"
#include "wml_utils.hpp"

frame::frame(wml::const_node_ptr node)
   : id_(node->name()),
     texture_(graphics::texture::get(node->attr("image"), node->attr("image_formula"))),
     collide_rect_(node->has_attr("collide") ? rect(node->attr("collide")) :
	               rect(wml::get_int(node, "collide_x"),
                        wml::get_int(node, "collide_y"),
                        wml::get_int(node, "collide_w"),
                        wml::get_int(node, "collide_h"))),
	 hit_rect_(node->has_attr("hit") ? rect(node->attr("hit")) :
	               rect(wml::get_int(node, "hit_x"),
				        wml::get_int(node, "hit_y"),
				        wml::get_int(node, "hit_w"),
				        wml::get_int(node, "hit_h"))),
	 platform_rect_(node->has_attr("platform") ? rect(node->attr("platform")) :
	                rect(wml::get_int(node, "platform_x"),
	                     wml::get_int(node, "platform_y"),
	                     wml::get_int(node, "platform_w"), 1)),
	 img_rect_(node->has_attr("rect") ? rect(node->attr("rect")) :
	           rect(wml::get_int(node, "x"),
	                wml::get_int(node, "y"),
	                wml::get_int(node, "w", texture_.width()),
	                wml::get_int(node, "h", texture_.height()))),
	 feet_x_(wml::get_int(node, "feet_x")),
	 feet_y_(wml::get_int(node, "feet_y")),
	 accel_x_(wml::get_int(node, "accel_x", INT_MIN)),
	 accel_y_(wml::get_int(node, "accel_y", INT_MIN)),
	 velocity_x_(wml::get_int(node, "velocity_x", INT_MIN)),
	 velocity_y_(wml::get_int(node, "velocity_y", INT_MIN)),
	 nframes_(wml::get_int(node, "frames", 1)),
	 nframes_per_row_(wml::get_int(node, "frames_per_row", -1)),
	 frame_time_(wml::get_int(node, "duration", -1)),
	 reverse_frame_(wml::get_bool(node, "reverse")),
	 play_backwards_(wml::get_bool(node, "play_backwards")),
	 scale_(wml::get_int(node, "scale", 2)),
	 pad_(wml::get_int(node, "pad")),
	 rotate_(wml::get_int(node, "rotate")),
	 blur_(wml::get_int(node, "blur")),
	 rotate_on_slope_(wml::get_bool(node, "rotate_on_slope")),
	 damage_(wml::get_int(node, "damage")),
	 sounds_(util::split(node->attr("sound")))
{
	std::vector<std::string> hit_frames = util::split((*node)["hit_frames"]);
	foreach(const std::string& f, hit_frames) {
		hit_frames_.push_back(boost::lexical_cast<int>(f));
	}

	const std::string& events = node->attr("events");
	if(!events.empty()) {
		//events are in the format time0:time1:...:timen:event0,time0:time1:...:timen:event1,...
		std::vector<std::string> event_vector = util::split(events);
		std::map<int, std::string> event_map;
		foreach(const std::string& e, event_vector) {
			std::vector<std::string> time_event = util::split(e, ':');
			if(time_event.size() < 2) {
				continue;
			}

			const std::string& event = time_event.back();

			for(unsigned int n = 0; n < time_event.size() - 1; ++n) {
				const int time = atoi(time_event[n].c_str());
				event_map[time] = event;
			}
		}

		typedef std::pair<int,std::string> event_pair;
		foreach(const event_pair& p, event_map) {
			event_frames_.push_back(p.first);
			event_names_.push_back(p.second);
		}
	}
}

void frame::play_sound(const void* object) const
{
	if (sounds_.empty() == false){
		int randomNum = rand()%sounds_.size();  //like a 1d-size die
		if(sounds_[randomNum].empty() == false) {
			std::cerr << "PLAY SOUND: '" << sounds_[randomNum] << "'\n";
			sound::play(sounds_[randomNum], object);
		}
	}
}

bool frame::is_alpha(int x, int y, int time, bool face_right) const
{
	if(x < 0 || y < 0 || x >= width() || y >= height()) {
		return true;
	}

	GLfloat rect[4];
	get_rect_in_texture(time, face_right, &rect[0]);
	
	const GLfloat xratio1 = GLfloat(width() - x)/width();
	const GLfloat xratio2 = GLfloat(x)/width();
	const GLfloat yratio1 = GLfloat(height() - y)/height();
	const GLfloat yratio2 = GLfloat(y)/height();

	const GLfloat u = xratio1*rect[0] + xratio2*rect[2];
	const GLfloat v = yratio1*rect[1] + yratio2*rect[3];

	ASSERT_GE(u, 0.0);
	ASSERT_GE(v, 0.0);
	ASSERT_LE(u, 1.0);
	ASSERT_LE(v, 1.0);

	return texture_.is_alpha((texture_.width()-1)*u, (texture_.height()-1)*v);
}

void frame::draw(int x, int y, bool face_right, bool upside_down, int time, int rotate) const
{
	GLfloat rect[4];
	get_rect_in_texture(time, face_right, &rect[0]);
	
	//the last 4 params are the rectangle of the single, specific frame
	graphics::blit_texture(texture_, x, y, width()*(face_right ? 1 : -1), height()*(upside_down ? -1 : 1), rotate + (face_right ? rotate_ : -rotate_),
	                       rect[0], rect[1], rect[2], rect[3]);

}

void frame::get_rect_in_texture(int time, bool face_right, GLfloat* output_rect) const
{
	//picks out a single frame to draw from a whole animation, based on time
	const int current_col = (nframes_per_row_ > 0) ? (frame_number(time) % nframes_per_row_) : frame_number(time) ;
	const int current_row = (nframes_per_row_ > 0) ? (frame_number(time)/nframes_per_row_) : 0 ;

	output_rect[0] = GLfloat(img_rect_.x() + current_col*(img_rect_.w()+pad_))/GLfloat(texture_.width());
	output_rect[1] = GLfloat(img_rect_.y() + ((img_rect_.h()+pad_) * current_row)) / GLfloat(texture_.height());
	output_rect[2] = GLfloat(img_rect_.x() + current_col*(img_rect_.w()+pad_) + img_rect_.w())/GLfloat(texture_.width());
	output_rect[3] = GLfloat(img_rect_.y() + ((img_rect_.h()+pad_) * current_row) + img_rect_.h())/GLfloat(texture_.height());
}

int frame::duration() const
{
	return (nframes_ + (reverse_frame_ ? nframes_ : 0))*frame_time_;
}

bool frame::hit(int time_in_frame) const
{
	if(hit_frames_.empty()) {
		return false;
	}

	return std::find(hit_frames_.begin(), hit_frames_.end(), frame_number(time_in_frame)) != hit_frames_.end();
}

int frame::frame_number(int time) const
{
	if(play_backwards_){
		int frame_num = nframes_-1;
		if(frame_time_ > 0 && nframes_ >= 1) {
			if(time >= duration()) {
				if(reverse_frame_){
					frame_num = nframes_-1;
				}else{	
					frame_num = 0;
				}
			} else {
				frame_num = nframes_-1 - time/frame_time_;
			}
			
			//if we are in reverse now
			if(frame_num < 0) {
				frame_num = -frame_num - 1;
			}
		}
		
		return frame_num;
	} else {
		int frame_num = 0;
		if(frame_time_ > 0 && nframes_ >= 1) {
			if(time >= duration()) {
				frame_num = nframes_-1;
			} else {
				frame_num = time/frame_time_;
			}
			
			//if we are in reverse now
			if(frame_num >= nframes_) {
				frame_num = nframes_ - 1 - (frame_num - nframes_);
			}
		}
		
		return frame_num;
	}
}

const std::string* frame::get_event(int time_in_frame) const
{
	if(event_frames_.empty()) {
		return NULL;
	}

	std::vector<int>::const_iterator i = std::find(event_frames_.begin(), event_frames_.end(), time_in_frame);
	if(i == event_frames_.end()) {
		return NULL;
	}

	return &event_names_[i - event_frames_.begin()];
}
