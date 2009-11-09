#ifndef FRAME_HPP_INCLUDED
#define FRAME_HPP_INCLUDED

#include <string>
#include <vector>

#include "geometry.hpp"
#include "solid_map_fwd.hpp"
#include "texture.hpp"
#include "wml_node_fwd.hpp"

class frame
{
public:
	struct collision_area {
		std::string name;
		rect area;
	};

	explicit frame(wml::const_node_ptr node);

	//ID of the frame. Not unique, but is the name of the element the frame
	//came from. Useful to tell what kind of frame it is.
	const std::string& id() const { return id_; }

	//play a sound. 'object' is just the address of the object playing the
	//sound, useful if the sound is later cancelled.
	void play_sound(const void* object=NULL) const;
	bool is_alpha(int x, int y, int time, bool face_right) const;
	void draw(int x, int y, bool face_right=true, bool upside_down=false, int time=0, int rotate=0) const;
	void set_image_as_solid();
	const_solid_info_ptr solid() const { return solid_; }
	int collide_x() const { return collide_rect_.x()*scale_; }
	int collide_y() const { return collide_rect_.y()*scale_; }
	int collide_w() const { return collide_rect_.w()*scale_; }
	int collide_h() const { return collide_rect_.h()*scale_; }
	int hit_x() const { return hit_rect_.x()*scale_; }
	int hit_y() const { return hit_rect_.y()*scale_; }
	int hit_w() const { return hit_rect_.w()*scale_; }
	int hit_h() const { return hit_rect_.h()*scale_; }
	int platform_x() const { return platform_rect_.x()*scale_; }
	int platform_y() const { return platform_rect_.y()*scale_; }
	int platform_w() const { return platform_rect_.w()*scale_; }
	bool has_platform() const { return platform_rect_.w() > 0; }
	int feet_x() const { return feet_x_*scale_; }
	int feet_y() const { return feet_y_*scale_; }
	int accel_x() const { return accel_x_; }
	int accel_y() const { return accel_y_; }
	int velocity_x() const { return velocity_x_; }
	int velocity_y() const { return velocity_y_; }
	int width() const { return img_rect_.w()*scale_; }
	int height() const { return img_rect_.h()*scale_; }
	int duration() const;
	bool hit(int time_in_frame) const;
	const graphics::texture& img() const { return texture_; }
	const rect& area() const { return img_rect_; }
	int blur() const { return blur_; }
	bool rotate_on_slope() const { return rotate_on_slope_; }
	int damage() const { return damage_; }

	const std::string* get_event(int time_in_frame) const;

	const std::vector<collision_area>& collision_areas() const { return collision_areas_; }
private:
	int frame_number(int time_in_frame) const;

	void get_rect_in_texture(int time, GLfloat* output_rect) const;
	void get_rect_in_frame_number(int nframe, GLfloat* output_rect) const;
	std::string id_;
	graphics::texture texture_;
	const_solid_info_ptr solid_;
	rect collide_rect_;
	rect hit_rect_;
	rect img_rect_;
	rect platform_rect_;
	std::vector<int> hit_frames_;
	int platform_x_, platform_y_, platform_w_;
	int feet_x_, feet_y_;
	int accel_x_, accel_y_;
	int velocity_x_, velocity_y_;
	int nframes_;
	int nframes_per_row_;
	int frame_time_;
	bool reverse_frame_;
	bool play_backwards_;
	int scale_;
	int pad_;
	int rotate_;
	int blur_;
	bool rotate_on_slope_;
	int damage_;

	std::vector<int> event_frames_;
	std::vector<std::string> event_names_;
	std::vector <std::string> sounds_;

	std::vector<collision_area> collision_areas_;

	void build_alpha();
	std::vector<bool> alpha_;

	GLuint shader_;
};

#endif
