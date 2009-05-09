#include <iostream>
#include <math.h>

#include "asserts.hpp"
#include "foreach.hpp"
#include "formatter.hpp"
#include "formula.hpp"
#include "level.hpp"
#include "raster.hpp"
#include "water.hpp"
#include "wml_node.hpp"
#include "wml_utils.hpp"
#include "color_utils.hpp"

water::water(wml::const_node_ptr water_node) :
  zorder_(wml::get_int(water_node, "zorder", 3)),
  current_x_formula_(game_logic::formula::create_optional_formula(water_node->attr("current_x_formula"))),
  current_y_formula_(game_logic::formula::create_optional_formula(water_node->attr("current_y_formula")))
{
	FOREACH_WML_CHILD(area_node, water_node, "area") {
		const rect r(area_node->attr("rect"));
		areas_.push_back(area(r));
	}
}

wml::node_ptr water::write() const
{
	wml::node_ptr result(new wml::node("water"));
	result->set_attr("zorder", formatter() << zorder_);
	foreach(const area& a, areas_) {
		wml::node_ptr node(new wml::node("area"));
		node->set_attr("rect", a.rect_.to_string());
	}

	return result;
}

void water::begin_drawing()
{
	foreach(area& a, areas_) {
		graphics::add_raster_distortion(&a.distortion_);
	}
}

void water::end_drawing() const
{
	foreach(const area& a, areas_) {
		graphics::remove_raster_distortion(&a.distortion_);
	}
}

/*
void water::set_surface_detection_rects(int zorder)
{
	const int offset = get_offset(zorder);
	foreach(area& a, areas_) {
		//detect drawing at the surface of the water.
		a.draw_detection_buf_.resize(a.rect_.w());
		memset(&a.draw_detection_buf_[0], 0, a.draw_detection_buf_.size());
		graphics::set_draw_detection_rect(rect(a.rect_.x(), a.rect_.y() + offset, a.rect_.w(), 1), &a.draw_detection_buf_[0]);
	}
}
*/

bool water::draw(int x, int y, int w, int h) const
{
	bool result = false;
	foreach(const area& a, areas_) {
		if(draw_area(a, x, y, w, h)) {
			result = true;
		}
	}

	end_drawing();

	return result;
}

void water::add_wave(const point& p, double xvelocity, double height, double length, double delta_height, double delta_length)
{
	std::cerr << "water::add_wave\n";
	foreach(area& a, areas_) {
		if(point_in_rect(p, a.rect_)) {
			wave wv = { p.x, xvelocity, height, length, delta_height, delta_length };
			a.waves_.push_back(wv);
			std::cerr << "adding wave...\n";
			return;
		}
	}
}

bool water::draw_area(const water::area& a, int x, int y, int w, int h) const
{
	const SDL_Color waterline_color = {250, 240, 205, 255};
	const SDL_Color deepwater_color = {91, 169, 143, 153};
	const SDL_Rect waterline_rect = {a.rect_.x(), a.rect_.y(), a.rect_.w(), 2};
	const SDL_Rect underwater_rect = {a.rect_.x(), a.rect_.y(), a.rect_.w(), a.rect_.h()};

	bool draw_with_waves = false;
	if(a.waves_.empty() == false) {

		std::vector<int> heights(w);
		foreach(const wave& wv, a.waves_) {
			int begin_x = std::max<int>(wv.xpos - wv.length, x);
			int end_x = std::min<int>(wv.xpos + wv.length + 1, x + w);
			for(int xpos = begin_x; xpos < end_x; ++xpos) {
				const int distance = abs(wv.xpos - xpos);
				const double proportion = GLfloat(distance)/GLfloat(wv.length);

				const int index = xpos - x;
				ASSERT_INDEX_INTO_VECTOR(index, heights);
				heights[index] += wv.height*(cos(proportion*3.14)+1.0)/2.0;
			}
		}

		const int begin_x = std::max(x, a.rect_.x());
		const int end_x = std::min(x + w, a.rect_.x2());
		if(end_x > begin_x+1) {
			draw_with_waves = true;
			glDisable(GL_TEXTURE_2D);

			glColor4ub(91, 169, 143, 153);
			glBegin(GL_QUAD_STRIP);
			for(int xpos = begin_x; xpos != end_x; ++xpos) {
				const int index = xpos - x;
				ASSERT_INDEX_INTO_VECTOR(index, heights);
				const int ypos = a.rect_.y() - heights[index];

				glVertex3f(xpos, underwater_rect.y + underwater_rect.h, 0.0);
				glVertex3f(xpos, ypos, 0.0);
			}
			glEnd();

			glLineWidth(2.0);
			glBegin(GL_LINE_STRIP);
			glColor4f(1.0, 1.0, 1.0, 1.0);
			for(int xpos = begin_x; xpos != end_x; ++xpos) {
				const int index = xpos - x;
				ASSERT_INDEX_INTO_VECTOR(index, heights);
				const int ypos = a.rect_.y() - heights[index];
				glVertex3f(xpos, ypos, 0.0);
			}
			glEnd();

		}
	}

	if(draw_with_waves == false) {
		graphics::draw_rect(underwater_rect, deepwater_color, 153);
		glDisable(GL_TEXTURE_2D);
		glLineWidth(2.0);
		glColor4f(1.0, 1.0, 1.0, 1.0);
		glBegin(GL_LINE_STRIP);
		glVertex3f(waterline_rect.x, waterline_rect.y, 0);
		glVertex3f(waterline_rect.x + waterline_rect.w, waterline_rect.y, 0);
		glEnd();
	}

	glColor4f(1.0, 1.0, 1.0, 1.0);
	glEnable(GL_TEXTURE_2D);

	return true;
}

namespace {
bool wave_dead(const water::wave& w) {
	return w.height <= 0 || w.length <= 0;
}
}

void water::process(const level& lvl)
{
	foreach(area& a, areas_) {
		a.distortion_ = graphics::water_distortion(lvl.cycle(), a.rect_);
		foreach(wave& w, a.waves_) {
			w.process();
		}

		a.waves_.erase(std::remove_if(a.waves_.begin(), a.waves_.end(), wave_dead), a.waves_.end());
	}
}

void water::wave::process() {
	xpos += xvelocity;
	height -= delta_height;
	length += delta_length;
}

void water::get_current(const entity& e, int* velocity_x, int* velocity_y) const
{
	if(velocity_x && current_x_formula_) {
		*velocity_x += current_x_formula_->execute(e).as_int();
	}

	if(velocity_y && current_y_formula_) {
		*velocity_y += current_y_formula_->execute(e).as_int();
	}
}

bool water::is_underwater(const rect& r, rect* result_water_area) const
{
	const point p((r.x() + r.x2())/2, (r.y() + r.y2())/2);
	foreach(const area& a, areas_) {
		if(point_in_rect(p, a.rect_)) {
			if(result_water_area) {
				*result_water_area = a.rect_;
			}
			return true;
		}
	}

	return false;
}
