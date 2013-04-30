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

#include <assert.h>
#include <math.h>

#include <boost/array.hpp>

#include <vector>

#include "asserts.hpp"
#include "color_utils.hpp"
#include "draw_primitive.hpp"
#include "foreach.hpp"
#include "geometry.hpp"
#include "texture.hpp"

namespace graphics
{

namespace
{

typedef boost::array<GLfloat, 2> FPoint;

class arrow_primitive : public draw_primitive
{
public:
	explicit arrow_primitive(const variant& v);

private:

	void handle_draw() const;

	variant get_value(const std::string& key) const;
	void set_value(const std::string& key, const variant& value);

	void set_points(const variant& points);

	void curve(const FPoint& p1, const FPoint& p2, const FPoint& p3, std::vector<FPoint>* out) const;

	std::vector<FPoint> points_;
	GLfloat granularity_;
	int arrow_head_length_;
	GLfloat arrow_head_width_;
	graphics::color color_;
	int fade_in_length_;

	GLfloat width_base_, width_head_;

	mutable std::vector<GLfloat> uvarray_;
	mutable std::vector<GLfloat> varray_;
	mutable std::vector<unsigned char> carray_;

	texture texture_;
	GLfloat texture_scale_;

	void calculate_draw_arrays() const;
};

arrow_primitive::arrow_primitive(const variant& v)
  : granularity_(v["granularity"].as_decimal(decimal(0.005)).as_float()),
    arrow_head_length_(v["arrow_head_length"].as_int(10)),
    arrow_head_width_(v["arrow_head_width"].as_decimal(decimal(2.0)).as_float()),
	fade_in_length_(v["fade_in_length"].as_int(50)),
	width_base_(v["width_base"].as_decimal(decimal(12.0)).as_float()),
	width_head_(v["width_head"].as_decimal(decimal(5.0)).as_float())
{
	if(v.has_key("texture")) {
		texture_ = texture::get(v["texture"].as_string());
		texture_scale_ = v["texture_scale"].as_decimal(decimal(1.0)).as_float();
	}

	if(v.has_key("color")) {
		color_ = color(v["color"]);
	} else {
		color_ = color(200, 0, 0, 255);
	}

	set_points(v["points"]);
}

void arrow_primitive::calculate_draw_arrays() const
{
	if(!varray_.empty()) {
		return;
	}

	std::vector<FPoint> path;

	for(int n = 1; n < points_.size()-1; ++n) {
		std::vector<FPoint> new_path;
		curve(points_[n-1], points_[n], points_[n+1], &new_path);

		if(path.empty()) {
			path.swap(new_path);
		} else {
			assert(path.size() >= new_path.size());
			const int overlap = path.size()/2;
			for(int n = 0; n != overlap; ++n) {
				const float ratio = float(n)/float(overlap);
				FPoint& value = path[(path.size() - overlap) + n];
				FPoint new_value = new_path[n];
				value[0] = value[0]*(1.0-ratio) + new_value[0]*ratio;
				value[1] = value[1]*(1.0-ratio) + new_value[1]*ratio;
			}

			path.insert(path.end(), new_path.begin() + overlap, new_path.end());
		}
	}

	const GLfloat PathLength = path.size()-1;

	std::vector<FPoint> left_path, right_path;
	for(int n = 0; n < path.size()-1; ++n) {
		const FPoint& p = path[n];
		const FPoint& next = path[n+1];

		FPoint direction;
		for(int m = 0; m != 2; ++m) {
			direction[m] = next[m] - p[m];
		}

		const GLfloat vector_length = sqrt(direction[0]*direction[0] + direction[1]*direction[1]);
		if(vector_length == 0.0) {
			continue;
		}

		FPoint unit_direction;
		for(int m = 0; m != 2; ++m) {
			unit_direction[m] = direction[m]/vector_length;
		}
		
		FPoint normal_direction_left, normal_direction_right;
		normal_direction_left[0] = -unit_direction[1];
		normal_direction_left[1] = unit_direction[0];
		normal_direction_right[0] = unit_direction[1];
		normal_direction_right[1] = -unit_direction[0];

		const GLfloat ratio = n/PathLength;

		GLfloat arrow_width = width_base_ - (width_base_-width_head_)*ratio;

		const int time_until_end = path.size()-2 - n;
		if(time_until_end < arrow_head_length_) {
			arrow_width = arrow_head_width_*time_until_end;
		}

		FPoint left, right;
		for(int m = 0; m != 2; ++m) {
			left[m] = p[m] + normal_direction_left[m]*arrow_width;
			right[m] = p[m] + normal_direction_right[m]*arrow_width;
		}

		left_path.push_back(left);
		right_path.push_back(right);
	}

	for(int n = 0; n != left_path.size(); ++n) {
		varray_.push_back(left_path[n][0]);
		varray_.push_back(left_path[n][1]);
		varray_.push_back(right_path[n][0]);
		varray_.push_back(right_path[n][1]);

		uvarray_.push_back(n*texture_scale_);
		uvarray_.push_back(0.0);
		uvarray_.push_back(n*texture_scale_);
		uvarray_.push_back(1.0);

		for(int m = 0; m != 2; ++m) {
			carray_.push_back(color_.r());
			carray_.push_back(color_.g());
			carray_.push_back(color_.b());
			if(n < fade_in_length_) {
				carray_.push_back(int((GLfloat(color_.a())*GLfloat(n)*(255.0/GLfloat(fade_in_length_)))/255.0));
			} else {
				carray_.push_back(color_.a());
			}
		}
	}
}

void arrow_primitive::handle_draw() const
{
	if(points_.size() < 3) {
		return;
	}

	calculate_draw_arrays();

#if !defined(USE_GLES2)
	glEnableClientState(GL_COLOR_ARRAY);
	glDisable(GL_TEXTURE_2D);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	glVertexPointer(2, GL_FLOAT, 0, &varray_[0]);
	glColorPointer(4, GL_UNSIGNED_BYTE, 0, &carray_[0]);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, varray_.size()/2);

	glDisableClientState(GL_COLOR_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnable(GL_TEXTURE_2D);
#else
	gles2::manager gles2_manager(texture_.valid() ? gles2::get_texcol_shader() : gles2::get_simple_col_shader());

	if(texture_.valid()) {
		glActiveTexture(GL_TEXTURE0);
		texture_.set_as_current_texture();
		gles2::active_shader()->shader()->texture_array(2, GL_FLOAT, GL_FALSE, 0, &uvarray_[0]);
	}

	gles2::active_shader()->shader()->vertex_array(2, GL_FLOAT, GL_FALSE, 0, &varray_[0]);
	gles2::active_shader()->shader()->color_array(4, GL_UNSIGNED_BYTE, GL_TRUE, 0, &carray_[0]);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, varray_.size()/2);
#endif
}

variant arrow_primitive::get_value(const std::string& key) const
{
	if(key == "points") {
		std::vector<variant> result;
		foreach(const FPoint& p, points_) {
			std::vector<variant> pos;
			pos.push_back(variant(static_cast<int>(p[0])));
			pos.push_back(variant(static_cast<int>(p[1])));
			result.push_back(variant(&pos));
		}

		return variant(&result);
	}
	ASSERT_LOG(false, "ILLEGAL KEY IN ARROW: " << key);
	return variant();
}

void arrow_primitive::set_value(const std::string& key, const variant& value)
{
	if(key == "points") {
		set_points(value);
	} else if(key == "color") {
		color_ = graphics::color(value);
	} else if(key == "granularity") {
		granularity_ = value.as_decimal().as_float();
	} else if(key == "arrow_head_length") {
		arrow_head_length_ = value.as_int();
	} else if(key == "arrow_head_width") {
		arrow_head_width_ = value.as_decimal().as_float();
	} else if(key == "fade_in_length") {
		fade_in_length_ = value.as_int();
	} else if(key == "width_base") {
		width_base_ = value.as_decimal().as_float();
	} else if(key == "width_head") {
		width_head_ = value.as_decimal().as_float();
	} else {
		ASSERT_LOG(false, "ILLEGAL KEY IN ARROW: " << key);
	}

	varray_.clear();
	carray_.clear();
}

void arrow_primitive::set_points(const variant& points)
{
	ASSERT_LOG(points.is_list(), "arrow points is not a list: " << points.debug_location());

	points_.clear();

	for(int n = 0; n != points.num_elements(); ++n) {
		variant p = points[n];
		ASSERT_LOG(p.is_list() && p.num_elements() == 2, "arrow points in invalid format: " << points.debug_location() << " : " << p.write_json());
		FPoint point;
		point[0] = p[0].as_int();
		point[1] = p[1].as_int();
		points_.push_back(point);
	}
}

void arrow_primitive::curve(const FPoint& p0, const FPoint& p1, const FPoint& p2, std::vector<FPoint>* out) const
{
	for(float t = 0.0; t < 1.0 - granularity_; t += granularity_) {
		FPoint p;
		for(int n = 0; n != 2; ++n) {
			//formula for a bezier curve.
			p[n] = (1-t)*(1-t)*p0[n] + 2*(1-t)*t*p1[n] + t*t*p2[n];
		}

		out->push_back(p);
	}
}

}

draw_primitive_ptr draw_primitive::create(const variant& v)
{
	const std::string type = v["type"].as_string();
	if(type == "arrow") {
		return draw_primitive_ptr(new arrow_primitive(v));
	}

	ASSERT_LOG(false, "UNKNOWN DRAW PRIMITIVE TYPE: " << v["type"].as_string());
	return draw_primitive_ptr();
}

void draw_primitive::draw() const
{
	handle_draw();
}

}
