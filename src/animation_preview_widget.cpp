#ifndef NO_EDITOR
#include <boost/bind.hpp>

#include "animation_preview_widget.hpp"
#include "button.hpp"
#include "foreach.hpp"
#include "formatter.hpp"
#include "raster.hpp"
#include "texture.hpp"

namespace gui {

bool animation_preview_widget::is_animation(variant obj)
{
	return !obj.is_null() && obj["image"].is_string();
}

animation_preview_widget::animation_preview_widget(variant obj) : cycle_(0), zoom_label_(NULL), pos_label_(NULL), scale_(0), anchor_x_(-1), anchor_y_(-1), anchor_pad_(-1), dragging_sides_bitmap_(0)
{
	set_object(obj);
}

void animation_preview_widget::init()
{
	widgets_.clear();

	button* b = new button("+", boost::bind(&animation_preview_widget::zoom_in, this));
	b->set_loc(x() + 10, y() + height() - b->height() - 5);
	widgets_.push_back(widget_ptr(b));
	b = new button("-", boost::bind(&animation_preview_widget::zoom_out, this));
	b->set_loc(x() + 40, y() + height() - b->height() - 5);
	widgets_.push_back(widget_ptr(b));

	zoom_label_ = new label("Zoom: 100%");
	zoom_label_->set_loc(b->x() + b->width() + 10, b->y());
	widgets_.push_back(widget_ptr(zoom_label_));

	pos_label_ = new label("");
	pos_label_->set_loc(zoom_label_->x() + zoom_label_->width() + 8, zoom_label_->y());
	widgets_.push_back(widget_ptr(pos_label_));
}

void animation_preview_widget::set_object(variant obj)
{
	if(obj == obj_) {
		return;
	}

	obj_ = obj;
	frame_.reset(new frame(obj));
	cycle_ = 0;
}

void animation_preview_widget::process()
{
}

void animation_preview_widget::handle_draw() const
{
	int mousex, mousey;
	int mouse_buttons = SDL_GetMouseState(&mousex, &mousey);
	graphics::draw_rect(rect(x(),y(),width(),height()), graphics::color(0,0,0,196));
	rect image_area(x(), y(), (width()*3)/4, height() - 30);
	const graphics::texture image_texture(graphics::texture::get(obj_["image"].as_string()));
	if(image_texture.valid()) {
#ifndef SDL_VIDEO_OPENGL_ES
		const graphics::clip_scope clipping_scope(image_area.sdl_rect());
#endif // SDL_VIDEO_OPENGL_ES

		const bool view_locked = mouse_buttons && locked_focus_.w()*locked_focus_.h();

		rect focus_area(frame_->area().x(), frame_->area().y(),
		      (frame_->area().w() + frame_->pad())*frame_->num_frames_per_row(),
			  (frame_->area().h() + frame_->pad())*(frame_->num_frames()/frame_->num_frames_per_row() + (frame_->num_frames()%frame_->num_frames_per_row() ? 1 : 0)));

		if(view_locked) {
			focus_area = locked_focus_;
		} else {
			locked_focus_ = focus_area;
		}

		GLfloat scale = 2.0;
		for(int n = 0; n != abs(scale_); ++n) {
			scale *= (scale_ < 0 ? 0.5 : 2.0);
		}

		if(!view_locked) {
			while(image_area.w()*scale*2.0 < image_area.w() &&
			      image_area.h()*scale*2.0 < image_area.h()) {
				scale *= 2.0;
				scale_++;
				update_zoom_label();
			}
		
			while(focus_area.w()*scale > image_area.w() ||
			      focus_area.h()*scale > image_area.h()) {
				scale *= 0.5;
				scale_--;
				update_zoom_label();
			}
		}

		const int show_width = image_area.w()/scale;
		const int show_height = image_area.h()/scale;

		int x1 = focus_area.x() + (focus_area.w() - show_width)/2;
		int y1 = focus_area.y() + (focus_area.h() - show_height)/2;
		int x2 = x1 + show_width;
		int y2 = y1 + show_height;

		int xpos = image_area.x();
		int ypos = image_area.y();

		src_rect_ = rect(x1, y1, x2 - x1, y2 - y1);
		dst_rect_ = rect(xpos, ypos, (x2-x1)*scale, (y2-y1)*scale);

		graphics::blit_texture(image_texture, xpos, ypos,
		                       (x2-x1)*scale, (y2-y1)*scale, 0.0,
							   GLfloat(x1)/image_texture.width(),
							   GLfloat(y1)/image_texture.height(),
							   GLfloat(x2)/image_texture.width(),
							   GLfloat(y2)/image_texture.height());

		if(!mouse_buttons) {
			dragging_sides_bitmap_ = 0;
		}

		for(int n = 0; n != frame_->num_frames(); ++n) {
			const int row = n/frame_->num_frames_per_row();
			const int col = n%frame_->num_frames_per_row();
			const int x = xpos - x1*scale + (frame_->area().x() + col*(frame_->area().w()+frame_->pad()))*scale;
			const int y = ypos - y1*scale + (frame_->area().y() + row*(frame_->area().h()+frame_->pad()))*scale;
			const rect box(x, y, frame_->area().w()*scale, frame_->area().h()*scale);
			graphics::draw_hollow_rect(box.sdl_rect(), n == 0 ? graphics::color_yellow() : graphics::color_white(), frame_->frame_number(cycle_) == n ? 0xFF : 0x88);

			if(n == 0 && !mouse_buttons) {
				if(point_in_rect(point(mousex, mousey), rect(box.x(), box.y()-4, box.w(), 9))) {
					dragging_sides_bitmap_ |= TOP_SIDE;
					graphics::draw_rect(rect(box.x(), box.y()-1, box.w(), 2).sdl_rect(), graphics::color_red());
				} else if(point_in_rect(point(mousex, mousey), rect(box.x(), box.y2()-4, box.w(), 9))) {
					dragging_sides_bitmap_ |= BOTTOM_SIDE;
					graphics::draw_rect(rect(box.x(), box.y2()-1, box.w(), 2).sdl_rect(), graphics::color_red());
				}

				if(point_in_rect(point(mousex, mousey), rect(box.x()-4, box.y(), 9, box.h()))) {
					dragging_sides_bitmap_ |= LEFT_SIDE;
					graphics::draw_rect(rect(box.x()-1, box.y(), 2, box.h()).sdl_rect(), graphics::color_red());
				} else if(point_in_rect(point(mousex, mousey), rect(box.x2()-4, box.y(), 9, box.h()))) {
					dragging_sides_bitmap_ |= RIGHT_SIDE;
					graphics::draw_rect(rect(box.x2()-1, box.y(), 2, box.h()).sdl_rect(), graphics::color_red());
				}
			} else if(n != 0 && !mouse_buttons) {
				if(point_in_rect(point(mousex, mousey), box)) {
					dragging_sides_bitmap_ = PADDING;
					graphics::draw_rect(box.sdl_rect(), graphics::color_yellow(), 128);
				}
			}
		}

		if(anchor_x_ != -1 && !dragging_sides_bitmap_ &&
		   SDL_GetMouseState(&mousex, &mousey) &&
		   point_in_rect(point(mousex, mousey), dst_rect_)) {
			const point p1 = mouse_point_to_image_loc(point(mousex, mousey));
			const point p2 = mouse_point_to_image_loc(point(anchor_x_, anchor_y_));

			int xpos1 = xpos - x1*scale + p1.x*scale;
			int xpos2 = xpos - x1*scale + p2.x*scale;
			int ypos1 = ypos - y1*scale + p1.y*scale;
			int ypos2 = ypos - y1*scale + p2.y*scale;
			
			if(xpos2 < xpos1) {
				std::swap(xpos1, xpos2);
			}

			if(ypos2 < ypos1) {
				std::swap(ypos1, ypos2);
			}
			
			rect area(xpos1, ypos1, xpos2 - xpos1, ypos2 - ypos1);
			graphics::draw_hollow_rect(area.sdl_rect(), graphics::color_white());
		}
	}

	rect preview_area(x() + (width()*3)/4, y(), width()/4, height());
	GLfloat scale = 1.0;
	while(false && (frame_->width()*scale > preview_area.w() || frame_->height()*scale > preview_area.h())) {
		scale *= 0.5;
	}

	frame_->draw(
	  preview_area.x() + (preview_area.w() - frame_->width()*scale)/2,
	  preview_area.y() + (preview_area.h() - frame_->height()*scale)/2,
	  true, false, cycle_, 0, scale);
	if(++cycle_ >= frame_->duration()) {
		cycle_ = 0;
	}

	foreach(const_widget_ptr w, widgets_) {
		w->draw();
	}
}

point animation_preview_widget::mouse_point_to_image_loc(const point& p) const
{
	const double xpos = double(p.x - dst_rect_.x())/double(dst_rect_.w());
	const double ypos = double(p.y - dst_rect_.y())/double(dst_rect_.h());

	const int x = src_rect_.x() + (double(src_rect_.w()) + 1.0)*xpos;
	const int y = src_rect_.y() + (double(src_rect_.h()) + 1.0)*ypos;
	
	return point(x, y);
}

bool animation_preview_widget::handle_event(const SDL_Event& event, bool claimed)
{
	foreach(widget_ptr w, widgets_) {
		claimed = w->process_event(event, claimed) || claimed;
	}

	if(event.type == SDL_MOUSEMOTION) {
		const SDL_MouseMotionEvent& e = event.motion;
		point p(e.x, e.y);
		if(point_in_rect(p, dst_rect_)) {
			p = mouse_point_to_image_loc(p);
			pos_label_->set_text(formatter() << p.x << "," << p.y);
		}

		if(e.state && dragging_sides_bitmap_) {
			int delta_x = e.x - anchor_x_;
			int delta_y = e.y - anchor_y_;

			if(scale_ >= 0) {
				for(int n = 0; n < scale_+1; ++n) {
					delta_x >>= 1;
					delta_y >>= 1;
				}
			}

			int x1 = anchor_area_.x();
			int x2 = anchor_area_.x2();
			int y1 = anchor_area_.y();
			int y2 = anchor_area_.y2();

			if(dragging_sides_bitmap_&LEFT_SIDE) {
				x1 += delta_x;
				if(x1 > x2 - 1) {
					x1 = x2 - 1;
				}
			}

			if(dragging_sides_bitmap_&RIGHT_SIDE) {
				x2 += delta_x;
				if(x2 < x1 + 1) {
					x2 = x1 + 1;
				}
			}

			if(dragging_sides_bitmap_&TOP_SIDE) {
				y1 += delta_y;
				if(y1 > y2 - 1) {
					y1 = y2 - 1;
				}
			}

			if(dragging_sides_bitmap_&BOTTOM_SIDE) {
				y2 += delta_y;
				if(y2 < y1 + 1) {
					y2 = y1 + 1;
				}
			}

			const int width = x2 - x1;
			const int height = y2 - y1;

			rect area(x1, y1, width, height);
			if(area != frame_->area() && rect_handler_) {
				rect_handler_(area);
			}

			if(dragging_sides_bitmap_&PADDING && pad_handler_) {
				const int new_pad = anchor_pad_ + delta_x;
				if(new_pad != frame_->pad()) {
					pad_handler_(new_pad);
				}
			}
		}
	} else if(event.type == SDL_MOUSEBUTTONDOWN) {
		const SDL_MouseButtonEvent& e = event.button;
		const point p(e.x, e.y);
		anchor_area_ = frame_->area();
		anchor_pad_ = frame_->pad();
		if(point_in_rect(p, dst_rect_)) {
			claimed = true;
			anchor_x_ = e.x;
			anchor_y_ = e.y;
		} else {
			anchor_x_ = anchor_y_ = -1;
		}
	} else if(event.type == SDL_MOUSEBUTTONUP && anchor_x_ != -1) {
		const SDL_MouseButtonEvent& e = event.button;
		point anchor(anchor_x_, anchor_y_);
		point p(e.x, e.y);
		if(!dragging_sides_bitmap_ && point_in_rect(anchor, dst_rect_) && point_in_rect(p, dst_rect_)) {
			claimed = true;

			anchor = mouse_point_to_image_loc(anchor);
			p = mouse_point_to_image_loc(p);

			int x1 = anchor.x;
			int y1 = anchor.y;
			int x2 = p.x;
			int y2 = p.y;

			if(x2 < x1) {
				std::swap(x1, x2);
			}

			if(y2 < y1) {
				std::swap(y1, y2);
			}

			rect area(x1, y1, x2 - x1, y2 - y1);

			if(rect_handler_) {
				rect_handler_(area);
			}
		}

		anchor_x_ = anchor_y_ = -1;
	}

	return claimed;
}

void animation_preview_widget::zoom_in()
{
	++scale_;
	update_zoom_label();
}

void animation_preview_widget::zoom_out()
{
	--scale_;
	update_zoom_label();
}

void animation_preview_widget::update_zoom_label() const
{
	if(zoom_label_) {
		int percent = 100;
		for(int n = 0; n != abs(scale_); ++n) {
			if(scale_ > 0) {
				percent *= 2;
			} else {
				percent /= 2;
			}
		}

		zoom_label_->set_text(formatter() << "Zoom: " << percent << "%");
	}
}

void animation_preview_widget::set_rect_handler(boost::function<void(rect)> handler)
{
	rect_handler_ = handler;
}

void animation_preview_widget::set_pad_handler(boost::function<void(int)> handler)
{
	pad_handler_ = handler;
}

}

#endif // !NO_EDITOR

