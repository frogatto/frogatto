#include "asserts.hpp"
#include "bar_widget.hpp"
#include "raster.hpp"

namespace gui
{
	bar_widget::bar_widget(const variant& v, game_logic::formula_callable* e)
		: widget(v, e), segments_(v["segments"].as_int(1)), 
		segment_length_(v["segment_length"].as_int(5)), 
		rotate_(GLfloat(v["rotation"].as_decimal().as_float())),
		tick_width_(v["tick_width"].as_int(1)), scale_(2.0f),
		drained_segments_(v["drained_segments"].as_int(0)), animating_(false),
		drain_rate_(v["drain_rate"].as_decimal(decimal(10.0)).as_float()),
		total_bar_length_(0), drained_bar_length_(0), active_bar_length_(0),
		left_cap_width_(0), right_cap_width_(0)
	{
		if(v.has_key("bar_color")) {
			bar_color_ = graphics::color(v["bar_color"]).as_sdl_color();
		} else {
			bar_color_ = graphics::color("red").as_sdl_color();
		}
		if(v.has_key("tick_color")) {
			tick_mark_color_ = graphics::color(v["tick_color"]).as_sdl_color();
		} else {
			tick_mark_color_ = graphics::color("black").as_sdl_color();
		}
		if(v.has_key("drained_bar_color")) {
			drained_bar_color_ = graphics::color(v["drained_bar_color"]).as_sdl_color();
		} else {
			drained_bar_color_ = graphics::color("black").as_sdl_color();
		}
		if(v.has_key("drained_tick_color")) {
			drained_tick_mark_color_ = graphics::color(v["drained_tick_color"]).as_sdl_color();
		} else {
			drained_tick_mark_color_ = graphics::color("white").as_sdl_color();
		}

		if(v.has_key("scale")) {
			scale_ = GLfloat(v["scale"].as_decimal().as_float());
		}

		ASSERT_LOG(v.has_key("bar"), "Missing 'bar' attribute");
		init_bar_section(v["bar"], &bar_);
		ASSERT_LOG(v.has_key("left_cap"), "Missing 'left_cap' attribute");
		init_bar_section(v["left_cap"], &left_cap_);
		ASSERT_LOG(v.has_key("right_cap"), "Missing 'right_cap' attribute");
		init_bar_section(v["right_cap"], &right_cap_);

		ASSERT_GT(segments_, 0);
		ASSERT_GT(segment_length_, 0);
		if(drained_segments_ > segments_) {
			drained_segments_ = segments_;
		}
		if(drained_segments_ < 0) {
			drained_segments_ = 0;
		}
		init();
	}

	bar_widget::~bar_widget()
	{
	}

	void bar_widget::init_bar_section(const variant&v, bar_section* b)
	{
		b->texture = graphics::texture::get(v["image"].as_string());
		if(v.has_key("area")) {
			ASSERT_LOG(v["area"].is_list() && v["area"].num_elements() == 4, "'area' attribute must be four element list.");
			b->area = rect(v["area"][0].as_int(), v["area"][1].as_int(), v["area"][2].as_int(), v["area"][3].as_int());
		} else {
			b->area = rect(0, 0, b->texture.width(), b->texture.height());
		}
	}

	void bar_widget::init()
	{
		left_cap_width_ = left_cap_.area.w() ? left_cap_.area.w()*scale_ : left_cap_.texture.width()*scale_;
		right_cap_width_ = right_cap_.area.w() ? right_cap_.area.w()*scale_ : right_cap_.texture.width()*scale_;

		total_bar_length_ = (segments_ * segment_length_ + (segments_-1) * tick_width_) * scale_;
		drained_bar_length_ = (drained_segments_ * segment_length_ + (drained_segments_-1) * tick_width_) * scale_;
		active_bar_length_ = ((segments_-drained_segments_) * segment_length_ + (segments_-(drained_segments_?drained_segments_:1)) * tick_width_) * scale_;
		int w = total_bar_length_ + left_cap_width_ + right_cap_width_;
		int h;
		if(height() == 0) {
			h = std::max(bar_.area.h(), std::max(left_cap_.area.h(), right_cap_.area.h()))*scale_;
		} else {
			h = height()*scale_;
		}
		set_dim(w, h);
	}

	void bar_widget::set_rotation(GLfloat rotate)
	{
		rotate_ = rotate;
	}

	variant bar_widget::get_value(const std::string& key) const
	{
		if(key == "segments") {
			return variant(segments_);
		} else if(key == "segment_length") {
			return variant(segment_length_);
		} else if(key == "tick_width") {
			return variant(tick_width_);
		} else if(key == "scale") {
			return variant(decimal(scale_));
		} else if(key == "drained") {
			return variant(drained_segments_);
		} else if(key == "drain_rate") {
			return variant(drain_rate_);
		}
		return widget::get_value(key);
	}

	void bar_widget::set_value(const std::string& key, const variant& value)
	{
		if(key == "segments") {
			segments_ = value.as_int();
			ASSERT_GE(segments_, 0);
			init();
		} else if(key == "segment_length") {
			segment_length_ = value.as_int();
			ASSERT_GT(segment_length_, 0);
			init();
		} else if(key == "tick_width") {
			tick_width_ = value.as_int();
			ASSERT_GT(tick_width_, 0);
			init();
		} else if(key == "scale") {
			scale_ = value.as_decimal().as_float();
			ASSERT_GT(scale_, 0.0f);
		} else if(key == "drain_rate") {
			drain_rate_ = value.as_decimal().as_float();
			ASSERT_GE(drain_rate_, 0.0);
		} else if(key == "drained") {
			drained_segments_ = value.as_int();
			if(drained_segments_ < 0) {
				drained_segments_ = 0;
			}
			if(drained_segments_ > segments_) {
				drained_segments_ = segments_;
			}
			init();
		}
		widget::set_value(key, value);
	}

	void bar_widget::draw_ticks(GLfloat x_offset, int segments, const SDL_Color& color) const
	{
		// tick marks
		if(segments > 1) {
			std::vector<GLfloat>& varray = graphics::global_vertex_array();
			varray.clear();
			for(int n = 1; n < segments; ++n) {
				GLfloat lx = x_offset + GLfloat((segment_length_ * n + (n - 1) * tick_width_ + 1) * scale_);
				varray.push_back(lx);
				varray.push_back(GLfloat(y()));
				varray.push_back(lx);
				varray.push_back(GLfloat(y()+height()));
			}
			glLineWidth(GLfloat(tick_width_) * scale_);
			glColor4ub(color.r, color.g, color.b, 255);
#if defined(USE_GLES2)
			gles2::manager gles2_manager(gles2::get_simple_shader());
			gles2::active_shader()->shader()->vertex_array(2, GL_FLOAT, 0, 0, &varray.front());
			glDrawArrays(GL_LINES, 0, varray.size()/2);
#else
			glDisable(GL_TEXTURE_2D);
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);
			glVertexPointer(2, GL_FLOAT, 0, &varray.front());
			glDrawArrays(GL_LINES, 0, varray.size()/2);
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			glEnable(GL_TEXTURE_2D);
#endif
			glLineWidth(1.0f);
		}
	}

	void bar_widget::handle_draw() const
	{
		int x_offset = x();
		{
			color_save_context color_saver;

			// draw color under end caps.
			graphics::draw_rect(rect(x()+scale_, y()+scale_, left_cap_width_-2*scale_, height()-2*scale_), graphics::color(bar_color_));
			graphics::draw_rect(rect(x()+left_cap_width_+total_bar_length_, y()+scale_, right_cap_width_-scale_, height()-2*scale_), graphics::color(drained_segments_ ? drained_bar_color_ : bar_color_));

			// background for active segments.
			graphics::draw_rect(rect(x()+left_cap_width_, y(), active_bar_length_, height()), graphics::color(bar_color_));
			// background for drained segments.
			if(drained_segments_) {
				graphics::draw_rect(rect(x()+active_bar_length_+left_cap_width_, y(), drained_bar_length_, height()), graphics::color(drained_bar_color_));
			}

			
			draw_ticks(x()+left_cap_width_, segments_-drained_segments_+(drained_segments_?1:0), tick_mark_color_);
			draw_ticks(x()+left_cap_width_+active_bar_length_, drained_segments_, drained_tick_mark_color_);
		}

		// left cap
		if(left_cap_.area.w() == 0) {
			graphics::blit_texture(left_cap_.texture, x_offset, y(), left_cap_width_, height(), rotate_);
		} else {
			graphics::blit_texture(left_cap_.texture, x_offset, y(), left_cap_width_, height(), rotate_,
				GLfloat(left_cap_.area.x())/left_cap_.texture.width(),
				GLfloat(left_cap_.area.y())/left_cap_.texture.height(),
				GLfloat(left_cap_.area.x2())/left_cap_.texture.width(),
				GLfloat(left_cap_.area.y2())/left_cap_.texture.height());
		}
		x_offset += left_cap_width_;
		// bar
		if(bar_.area.w() == 0) {
			graphics::blit_texture(bar_.texture, x_offset, y(), total_bar_length_, height(), rotate_);
		} else {
			graphics::blit_texture(bar_.texture, x_offset, y(), total_bar_length_, height(), rotate_,
				GLfloat(bar_.area.x())/bar_.texture.width(),
				GLfloat(bar_.area.y())/bar_.texture.height(),
				GLfloat(bar_.area.x2())/bar_.texture.width(),
				GLfloat(bar_.area.y2())/bar_.texture.height());
		}
		x_offset += total_bar_length_;

		// right cap
		if(right_cap_.area.w() == 0) {
			graphics::blit_texture(left_cap_.texture, x_offset, y(), right_cap_width_, height(), rotate_);
		} else {
			graphics::blit_texture(right_cap_.texture, x_offset, y(), right_cap_width_, height(), rotate_,
				GLfloat(right_cap_.area.x())/right_cap_.texture.width(),
				GLfloat(right_cap_.area.y())/right_cap_.texture.height(),
				GLfloat(right_cap_.area.x2())/right_cap_.texture.width(),
				GLfloat(right_cap_.area.y2())/right_cap_.texture.height());
		}
	}

	bool bar_widget::handle_event(const SDL_Event& event, bool claimed)
	{
		return claimed;
	}

}