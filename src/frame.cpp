#include <algorithm>
#include <iostream>

#include <boost/lexical_cast.hpp>

#include "asserts.hpp"
#include "foreach.hpp"
#include "frame.hpp"
#include "object_events.hpp"
#include "raster.hpp"
#include "solid_map.hpp"
#include "sound.hpp"
#include "string_utils.hpp"
#include "surface_formula.hpp"
#include "texture.hpp"
#include "wml_node.hpp"
#include "wml_utils.hpp"

frame::frame(wml::const_node_ptr node)
   : id_(node->attr("id")),
     variant_id_(id_),
     enter_event_id_(get_object_event_id("enter_" + id_ + "_anim")),
	 end_event_id_(get_object_event_id("end_" + id_ + "_anim")),
	 leave_event_id_(get_object_event_id("leave_" + id_ + "_anim")),
	 process_event_id_(get_object_event_id("process_" + id_)),
     texture_(graphics::texture::get(node->attr("image"), node->attr("image_formula"))),
	 solid_(solid_info::create(node)),
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
	                wml::get_int(node, "w"),
	                wml::get_int(node, "h"))),
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

	static const std::string AreaPostfix = "_area";
	for(wml::node::const_attr_iterator i = node->begin_attr(); i != node->end_attr(); ++i) {
		const std::string& attr = i->first;
		if(attr.size() <= AreaPostfix.size() || std::equal(AreaPostfix.begin(), AreaPostfix.end(), attr.end() - AreaPostfix.size()) == false || attr == "solid_area" ||attr == "platform_area") {
			continue;
		}

		const std::string area_id = std::string(attr.begin(), attr.end() - AreaPostfix.size());

		bool solid = false;
		std::string value = i->second;

		static const std::string SolidStr = "solid:";
		if(value.size() > SolidStr.size() && std::equal(SolidStr.begin(), SolidStr.end(), value.begin())) {
			solid = true;
			value.erase(value.begin(), value.begin() + SolidStr.size());
		}

		rect r;
		if(value == "none") {
			continue;
		} else if(value == "all") {
			r = rect(0, 0, width(), height());
		} else {
			r = rect(value);
			r = rect(r.x()*scale_, r.y()*scale_, r.w()*scale_, r.h()*scale_);
		}
		collision_area area = { area_id, r, solid };
		collision_areas_.push_back(area);
	}

	if(node->has_attr("frame_info")) {
		const std::vector<int> values = wml::get_vector_int(node, "frame_info");
		ASSERT_EQ(values.size()%8, 0);
		std::vector<int>::const_iterator i = values.begin();
		while(i != values.end()) {
			frame_info info;
			info.x_adjust = *i++;
			info.y_adjust = *i++;
			info.x2_adjust = *i++;
			info.y2_adjust = *i++;
			const int x = *i++;
			const int y = *i++;
			const int w = *i++;
			const int h = *i++;
			info.area = rect(x, y, w, h);
			frames_.push_back(info);
			ASSERT_EQ(intersection_rect(info.area, rect(0, 0, texture_.width(), texture_.height())), info.area);
			ASSERT_EQ(w + (info.x_adjust + info.x2_adjust), img_rect_.w());
			ASSERT_EQ(h + (info.y_adjust + info.y2_adjust), img_rect_.h());

		}

		ASSERT_EQ(frames_.size(), nframes_);

		build_alpha_from_frame_info();
	} else {
		build_alpha();
	}
}

void frame::set_image_as_solid()
{
	solid_ = solid_info::create_from_texture(texture_, img_rect_);
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

void frame::build_alpha_from_frame_info()
{
	if(!texture_.valid()) {
		return;
	}

	alpha_.resize(nframes_*img_rect_.w()*img_rect_.h(), true);
	for(int n = 0; n < nframes_; ++n) {
		const rect& area = frames_[n].area;
		int dst_index = frames_[n].y_adjust*img_rect_.w()*nframes_ + n*img_rect_.w() + frames_[n].x_adjust;
		for(int y = 0; y != area.h(); ++y) {
			ASSERT_INDEX_INTO_VECTOR(dst_index, alpha_);
			std::vector<bool>::iterator dst = alpha_.begin() + dst_index;

			ASSERT_LT(area.x(), texture_.width());
			ASSERT_LE(area.x() + area.w(), texture_.width());
			ASSERT_LT(area.y() + y, texture_.height());
			std::vector<bool>::const_iterator src = texture_.get_alpha_row(area.x(), area.y() + y);

			std::copy(src, src + area.w(), dst);
			
			dst_index += img_rect_.w()*nframes_;
		}
	}
}

void frame::build_alpha()
{
	frames_.resize(nframes_);
	if(!texture_.valid()) {
		return;
	}

	alpha_.resize(nframes_*img_rect_.w()*img_rect_.h(), true);

	for(int n = 0; n < nframes_; ++n) {
		const int current_col = (nframes_per_row_ > 0) ? (n% nframes_per_row_) : n;
		const int current_row = (nframes_per_row_ > 0) ? (n/nframes_per_row_) : 0;
		const int xbase = img_rect_.x() + current_col*(img_rect_.w()+pad_);
		const int ybase = img_rect_.y() + current_row*(img_rect_.h()+pad_);

		if(xbase < 0 || ybase < 0 || xbase + img_rect_.w() > texture_.width() ||
		   ybase + img_rect_.h() > texture_.height()) {
			std::cerr << "IMAGE RECT FOR FRAME '" << id_ << "' #" << n << ": " << img_rect_.x() << " + " << current_col << " * (" << img_rect_.w() << "+" << pad_ << ") IS INVALID: " << xbase << ", " << ybase << ", " << (xbase + img_rect_.w()) << ", " << (ybase + img_rect_.h()) << " / " << texture_.width() << "," << texture_.height() << "\n";
			throw error();
		}

		for(int y = 0; y != img_rect_.h(); ++y) {
			const int dst_index = y*img_rect_.w()*nframes_ + n*img_rect_.w();
			ASSERT_INDEX_INTO_VECTOR(dst_index, alpha_);

			std::vector<bool>::iterator dst = alpha_.begin() + dst_index;

			std::vector<bool>::const_iterator src = texture_.get_alpha_row(xbase, ybase + y);
			std::copy(src, src + img_rect_.w(), dst);
		}

		//now calculate if the actual frame we should be using for drawing
		//is smaller than the outer rectangle, so we can save on drawing space
		frame_info& f = frames_[n];
		f.area = rect(xbase, ybase, img_rect_.w(), img_rect_.h());
		int top;
		for(top = 0; top != img_rect_.h(); ++top) {
			const std::vector<bool>::const_iterator a = texture_.get_alpha_row(xbase, ybase + top);
			if(std::find(a, a + img_rect_.w(), false) != a + img_rect_.w()) {
				break;
			}
		}

		int bot;
		for(bot = img_rect_.h(); bot > 0; --bot) {
			const std::vector<bool>::const_iterator a = texture_.get_alpha_row(xbase, ybase + bot-1);
			if(std::find(a, a + img_rect_.w(), false) != a + img_rect_.w()) {
				break;
			}
		}

		int left;
		for(left = 0; left < img_rect_.w(); ++left) {
			std::vector<bool>::const_iterator a = texture_.get_alpha_row(xbase + left, ybase);

			bool has_opaque = false;
			for(int n = 0; n != img_rect_.h(); ++n) {
				if(!*a) {
					has_opaque = true;
				}
				if(n+1 != img_rect_.h()) {
					a += texture_.width();
				}
			}

			if(has_opaque) {
				break;
			}
		}

		int right;
		for(right = img_rect_.w(); right > 0; --right) {
			std::vector<bool>::const_iterator a = texture_.get_alpha_row(xbase + right-1, ybase);

			bool has_opaque = false;
			for(int n = 0; n != img_rect_.h(); ++n) {
				if(!*a) {
					has_opaque = true;
				}

				if(n+1 != img_rect_.h()) {
					a += texture_.width();
				}
			}

			if(has_opaque) {
				break;
			}
		}

		if(right < left) {
			right = left;
		}

		if(bot < top) {
			bot = top;
		}

		f.x_adjust = left;
		f.y_adjust = top;
		f.x2_adjust = img_rect_.w() - right;
		f.y2_adjust = img_rect_.h() - bot;
		f.area = rect(xbase + left, ybase + top, right - left, bot - top);
		ASSERT_EQ(f.area.w() + f.x_adjust + f.x2_adjust, img_rect_.w());
		ASSERT_EQ(f.area.h() + f.y_adjust + f.y2_adjust, img_rect_.h());
	}
}

bool frame::is_alpha(int x, int y, int time, bool face_right) const
{
	if(alpha_.empty()) {
		return true;
	}

	if(face_right == false) {
		x = width() - x - 1;
	}

	if(x < 0 || y < 0 || x >= width() || y >= height()) {
		return true;
	}

	x /= scale_;
	y /= scale_;

	const int nframe = frame_number(time);
	x += nframe*img_rect_.w();
	
	const int index = y*img_rect_.w()*nframes_ + x;
	ASSERT_INDEX_INTO_VECTOR(index, alpha_);
	return alpha_[index];
}

void frame::draw_into_blit_queue(graphics::blit_queue& blit, int x, int y, bool face_right, bool upside_down, int time) const
{
	const frame_info* info = NULL;
	GLfloat rect[4];
	get_rect_in_texture(time, &rect[0], info);

	x += (face_right ? info->x_adjust : info->x2_adjust)*scale_;
	y += (info->y_adjust)*scale_;
	const int w = info->area.w()*scale_*(face_right ? 1 : -1);
	const int h = info->area.h()*scale_*(upside_down ? -1 : 1);

	blit.set_texture(texture_.get_id());

	blit.add(x, y, rect[0], rect[1]);
	blit.add(x + w, y, rect[2], rect[1]);
	blit.add(x, y + h, rect[0], rect[3]);
	blit.add(x + w, y + h, rect[2], rect[3]);
}

void frame::draw(int x, int y, bool face_right, bool upside_down, int time, int rotate) const
{
	const frame_info* info = NULL;
	GLfloat rect[4];
	get_rect_in_texture(time, &rect[0], info);

	x += (face_right ? info->x_adjust : info->x2_adjust)*scale_;
	y += info->y_adjust*scale_;
	const int w = info->area.w()*scale_*(face_right ? 1 : -1);
	const int h = info->area.h()*scale_*(upside_down ? -1 : 1);

	if(rotate == 0) {
		//if there is no rotation, then we can make a much simpler call
		graphics::queue_blit_texture(texture_, x, y, w, h, rect[0], rect[1], rect[2], rect[3]);
		graphics::flush_blit_texture();
		return;
	}

	graphics::queue_blit_texture(texture_, x, y, w, h, rotate, rect[0], rect[1], rect[2], rect[3]);
	graphics::flush_blit_texture();
}

void frame::draw(int x, int y, const rect& area, bool face_right, bool upside_down, int time, int rotate) const
{
	const frame_info* info = NULL;
	GLfloat rect[4];
	get_rect_in_texture(time, &rect[0], info);

	const int x_adjust = area.x();
	const int y_adjust = area.y();
	const int w_adjust = area.w() - img_rect_.w();
	const int h_adjust = area.h() - img_rect_.h();

	const int w = info->area.w()*scale_*(face_right ? 1 : -1);
	const int h = info->area.h()*scale_*(upside_down ? -1 : 1);

	rect[0] += GLfloat(x_adjust)/GLfloat(texture_.width());
	rect[1] += GLfloat(y_adjust)/GLfloat(texture_.height());
	rect[2] += GLfloat(x_adjust + w_adjust)/GLfloat(texture_.width());
	rect[3] += GLfloat(y_adjust + h_adjust)/GLfloat(texture_.height());

	//the last 4 params are the rectangle of the single, specific frame
	graphics::blit_texture(texture_, x, y, (w + w_adjust*scale_)*(face_right ? 1 : -1), (h + h_adjust*scale_)*(upside_down ? -1 : 1), rotate + (face_right ? rotate_ : -rotate_),
	                       rect[0], rect[1], rect[2], rect[3]);
}

void frame::get_rect_in_texture(int time, GLfloat* output_rect, const frame_info*& info) const
{
	//picks out a single frame to draw from a whole animation, based on time
	get_rect_in_frame_number(frame_number(time), output_rect, info);
}

void frame::get_rect_in_frame_number(int nframe, GLfloat* output_rect, const frame_info*& info_result) const
{
	const frame_info& info = frames_[nframe];
	info_result = &info;

	const int current_col = (nframes_per_row_ > 0) ? (nframe % nframes_per_row_) : nframe ;
	const int current_row = (nframes_per_row_ > 0) ? (nframe/nframes_per_row_) : 0 ;

	//a tiny amount we subtract from the right/bottom side of the texture,
	//to avoid rounding errors in floating point going over the edge.
	//This seems like a kludge but I don't know of a better way to do it. :(
	const GLfloat TextureEpsilon = 0.0001;

	output_rect[0] = GLfloat(info.area.x())/GLfloat(texture_.width()) + TextureEpsilon;
	output_rect[1] = GLfloat(info.area.y()) / GLfloat(texture_.height());
	output_rect[2] = GLfloat(info.area.x() + info.area.w())/GLfloat(texture_.width()) - TextureEpsilon;
	output_rect[3] = GLfloat(info.area.y() + info.area.h())/GLfloat(texture_.height()) - TextureEpsilon;
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
