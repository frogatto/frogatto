#ifndef NO_EDITOR
#include <boost/bind.hpp>

#include "animation_preview_widget.hpp"
#include "button.hpp"
#include "foreach.hpp"
#include "formatter.hpp"
#include "pathfinding.hpp"
#include "raster.hpp"
#include "solid_map.hpp"
#include "surface_cache.hpp"
#include "texture.hpp"

namespace {
using graphics::surface;
const unsigned char RedBorder[] = {0xf9, 0x30, 0x3d};
const unsigned char BackgroundColor[] = {0x6f, 0x6d, 0x51};

bool is_pixel_border(const surface& s, int x, int y)
{
	if(x < 0 || y < 0 || x >= s->w || y >= s->h) {
		return false;
	}

	unsigned char* pixel = reinterpret_cast<unsigned char*>(s->pixels) + y*s->pitch + x*4;
	for(int n = 0; n != 3; ++n) {
		if(pixel[n] != RedBorder[n]) {
			return false;
		}
	}

	return true;
}

bool is_pixel_alpha(const surface& s, const point& p)
{
	unsigned char* pixel = reinterpret_cast<unsigned char*>(s->pixels) + p.y*s->pitch + p.x*4;
	if(pixel[3] == 0) {
		return true;
	}
	for(int n = 0; n != 3; ++n) {
		if(pixel[n] != BackgroundColor[n]) {
			return false;
		}
	}

	return true;
}

rect get_border_rect(const surface& s, int x, int y)
{
	int w = 0, h = 0;
	while(is_pixel_border(s, x + w + 1, y)) {
		++w;
	}

	while(is_pixel_border(s, x, y + h + 1) &&
	      is_pixel_border(s, x + w, y + h + 1)) {
		++h;
	}

	if(w == 0 || h == 0) {
		return rect();
	}

	return rect(x+1, y+1, w-1, h-1);
}

int path_cost_fn(const surface& s, const point& p1, const point& p2) {
    bool a1 = is_pixel_alpha(s, p1);
    bool a2 = is_pixel_alpha(s, p2);
    if(a1 && a2) return 2;
    else if(a1 ^ a2) return 1;
	return 0;
}

rect get_border_rect_heuristic_search(const surface& s, int ox, int oy, int max_cost) 
{
	int x1 = INT_MAX, y1 = INT_MAX, x2 = INT_MIN, y2 = INT_MIN;
	if(ox < 0 || oy < 0 || ox >= s->w || oy >= s->h) {
		return rect::from_coordinates(ox,oy,ox+1,oy+1);
	}
	const rect r(0, 0, s->w, s->h);

	if(is_pixel_alpha(s, point(ox, oy))) {
		return rect::from_coordinates(ox,oy,ox+1,oy+1);
	}

	typedef pathfinding::graph_node<point, int> graph_node;
	typedef graph_node::graph_node_ptr graph_node_ptr;
	typedef std::map<point, graph_node_ptr> graph_node_list;
	graph_node_list node_list;
	std::deque<graph_node_ptr> open_list;
	typedef std::pair<point, bool> reachable_node;
	std::vector<reachable_node> reachable;

	bool searching = true;
	try {
		graph_node_ptr current = graph_node_ptr(new graph_node(point(ox, oy)));
		current->set_cost(0, 0);
		current->set_on_open_list(true);
		open_list.push_back(current);
		node_list[point(ox, oy)] = current;

		while(searching && !open_list.empty()) {
			current = open_list.front(); open_list.pop_front();
			current->set_on_open_list(false);
			current->set_on_closed_list(true);
			if(current->G() <= max_cost) {
				reachable.push_back(reachable_node(current->get_node_value(), 
					is_pixel_alpha(s, current->get_node_value())));
			}
			foreach(const point& p, pathfinding::get_neighbours_from_rect(current->get_node_value(), 1, 1, r)) {
				graph_node_list::const_iterator neighbour_node = node_list.find(p);
				int g_cost = path_cost_fn(s, p, current->get_node_value()) + current->G();
				if(neighbour_node == node_list.end()) {
					graph_node_ptr new_node = graph_node_ptr(new graph_node(point(p.x, p.y)));
						new_node->set_parent(current);
						new_node->set_cost(g_cost, 0);
						new_node->set_on_open_list(true);
						node_list[p] = new_node;
						if(g_cost > max_cost) {
							new_node->set_on_closed_list(true);
						} else {
							new_node->set_on_open_list(true);
							open_list.push_back(new_node);
						}
				} else if(neighbour_node->second->on_closed_list() || neighbour_node->second->on_open_list()) {
					if(g_cost < neighbour_node->second->G()) {
						neighbour_node->second->G(g_cost);
						neighbour_node->second->set_parent(current);
					}
				} else {
					throw "Path error node on list, but not on open or closed lists";
				}
			}
		}
	} catch(...) {
		std::cerr << "get_border_rect_heuristic_search(): Caught exception" << std::endl;
	}

	foreach(const reachable_node& rn, reachable) {
		if(rn.second) {
			if(rn.first.x < x1) x1 = rn.first.x;
			if(rn.first.x > x2) x2 = rn.first.x;
			if(rn.first.y < y1) y1 = rn.first.y;
			if(rn.first.y > y2) y2 = rn.first.y;
		}
	}

	std::cerr << "CALC RECT " << x1 << "," << y1 << "," << x2 << "," << y2 << std::endl;
	//std::cerr << "PIXEL: 0x" << std::hex << int(pixel[0]) << ",0x" << int(pixel[1]) << ",0x" << int(pixel[2]) << ",0x" << int(pixel[3]) << std::endl;
	return rect::from_coordinates(x1, y1, x2, y2);
}

rect get_border_rect_around_loc(const surface& s, int ox, int oy)
{
	int x = ox, y = oy;
	std::cerr << "SEARCHING FOR BORDER AROUND " << x << "," << y << "\n";
	while(y >= 0 && !is_pixel_border(s, x, y)) {
		--y;
	}

	while(x >= 0 && is_pixel_border(s, x, y)) {
		--x;
	}

	++x;

	std::cerr << "STEPPED TO " << x << "," << y << "\n";

	if(y >= 0 && is_pixel_border(s, x, y)) {
		std::cerr << "RETURNING " << get_border_rect(s, x, y) << "\n";
		return get_border_rect(s, x, y);
	} else {
		std::cerr << "TRYING HEURISTIC SEARCH AROUND " << ox << "," << oy << std::endl;
		return get_border_rect_heuristic_search(s, ox, oy, 10);
	}
}

bool find_full_animation(const surface& s, const rect& r, int* pad, int* num_frames, int* frames_per_row)
{
	const int x = r.x() + r.w()/2;
	const int y = r.y() + r.h()/2;

	int next_x = x + r.w();
	if(next_x >= s->w) {
		std::cerr << "FAIL FIND " << next_x << " >= " << s->w << "\n";
		return false;
	}

	rect next_rect = get_border_rect_around_loc(s, next_x, y);
	std::cerr << "NEXT RECT: " << next_rect << " VS " << r << "\n";
	if(next_rect.w() != r.w() || next_rect.h() != r.h()) {
		return false;
	}

	*pad = next_rect.x() - r.x2();
	*num_frames = 2;

	std::vector<rect> rect_row;
	rect_row.push_back(r);
	rect_row.push_back(next_rect);

	std::cerr << "SETTING... " << get_border_rect_around_loc(s, next_x + r.w() + *pad, y) << " VS " << rect(next_rect.x() + next_rect.w() + *pad, y, r.w(), r.h()) << "\n";

	while(next_x + r.w() + *pad < s->w && get_border_rect_around_loc(s, next_x + r.w() + *pad, y) == rect(next_rect.x() + next_rect.w() + *pad, r.y(), r.w(), r.h())) {
			std::cerr << "ITER\n";
		*num_frames += 1;
		next_x += r.w() + *pad;
		next_rect = rect(next_rect.x() + next_rect.w() + *pad, r.y(), r.w(), r.h());
		rect_row.push_back(next_rect);
	}

	*frames_per_row = *num_frames;

	bool row_valid = true;
	while(row_valid) {
		int index = 0;
		foreach(rect& r, rect_row) {
			rect next_rect(r.x(), r.y() + r.h() + *pad, r.w(), r.h());
			std::cerr << "MATCHING: " << get_border_rect_around_loc(s, next_rect.x() + next_rect.w()/2, next_rect.y() + next_rect.h()/2) << " VS " << next_rect << "\n";
			if(next_rect.y2() >= s->h || get_border_rect_around_loc(s, next_rect.x() + next_rect.w()/2, next_rect.y() + next_rect.h()/2) != next_rect) {
				std::cerr << "MISMATCH: " << index << "/" << rect_row.size() << " -- " << get_border_rect_around_loc(s, next_rect.x() + next_rect.w()/2, next_rect.y() + next_rect.h()/2) << " VS " << next_rect << "\n";
				row_valid = false;
				break;
			}

			r = next_rect;
			++index;
		}

		if(row_valid) {
			*num_frames += *frames_per_row;
		}
	}

	return true;
}

}

namespace gui {

bool animation_preview_widget::is_animation(variant obj)
{
	return !obj.is_null() && obj["image"].is_string();
}

animation_preview_widget::animation_preview_widget(const variant& v, game_logic::formula_callable* e) 
	: widget(v,e), cycle_(0), zoom_label_(NULL), pos_label_(NULL), scale_(0), 
	anchor_x_(-1), anchor_y_(-1), anchor_pad_(-1), has_motion_(false), dragging_sides_bitmap_(0), 
	moving_solid_rect_(false), anchor_solid_x_(-1), anchor_solid_y_(-1)
{
	ASSERT_LOG(get_environment() != 0, "You must specify a callable environment");

	if(v.has_key("on_rect_change")) {
		rect_handler_ = boost::bind(&animation_preview_widget::rect_handler_delegate, this, _1);
		ffl_rect_handler_ = get_environment()->create_formula(v["on_rect_change"]);
	}
	if(v.has_key("on_pad_change")) {
		pad_handler_ = boost::bind(&animation_preview_widget::pad_handler_delegate, this, _1);
		ffl_pad_handler_ = get_environment()->create_formula(v["on_pad_change"]);
	}
	if(v.has_key("on_frames_change")) {
		num_frames_handler_ = boost::bind(&animation_preview_widget::num_frames_handler_delegate, this, _1);
		ffl_num_frames_handler_ = get_environment()->create_formula(v["on_frames_change"]);
	}
	if(v.has_key("on_frames_per_row_change")) {
		frames_per_row_handler_ = boost::bind(&animation_preview_widget::frames_per_row_handler_delegate, this, _1);
		ffl_frames_per_row_handler_ = get_environment()->create_formula(v["on_frames_per_row_change"]);
	}
	if(v.has_key("on_solid_change")) {
		solid_handler_ = boost::bind(&animation_preview_widget::solid_handler_delegate, this, _1, _2);
		ffl_solid_handler_ = get_environment()->create_formula(v["on_solid_change"]);
	}
	
	try {
		set_object(v);
	} catch(type_error&) {
	} catch(frame::error&) {
	} catch(validation_failure_exception&) {
	} catch(graphics::load_image_error&) {
	}
}

animation_preview_widget::animation_preview_widget(variant obj) : cycle_(0), zoom_label_(NULL), pos_label_(NULL), scale_(0), anchor_x_(-1), anchor_y_(-1), anchor_pad_(-1), has_motion_(false), dragging_sides_bitmap_(0), moving_solid_rect_(false), anchor_solid_x_(-1), anchor_solid_y_(-1)
{
	set_environment();
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

	b = new button("Reset", boost::bind(&animation_preview_widget::reset_rect, this));
	b->set_loc(pos_label_->x() + pos_label_->width() + 8, y() + height() - b->height() - 5);
	widgets_.push_back(widget_ptr(b));
}

void animation_preview_widget::set_object(variant obj)
{
	if(obj == obj_) {
		return;
	}

	frame* f = new frame(obj);

	obj_ = obj;
	frame_.reset(f);
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
	const graphics::texture image_texture(graphics::texture::get(obj_["image"].as_string(), graphics::texture::NO_STRIP_SPRITESHEET_ANNOTATIONS));
	if(image_texture.valid()) {
#ifndef SDL_VIDEO_OPENGL_ES
		const graphics::clip_scope clipping_scope(image_area.sdl_rect());
#endif // SDL_VIDEO_OPENGL_ES

		const bool view_locked = mouse_buttons && locked_focus_.w()*locked_focus_.h();

		rect focus_area;
		if(frame_->num_frames_per_row() == 0) {
			focus_area = rect();
		} else {
			focus_area = rect(frame_->area().x(), frame_->area().y(),
				  (frame_->area().w() + frame_->pad())*frame_->num_frames_per_row(),
				  (frame_->area().h() + frame_->pad())*(frame_->num_frames()/frame_->num_frames_per_row() + (frame_->num_frames()%frame_->num_frames_per_row() ? 1 : 0)));
		}

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
		if(x1 < 0) {
			x1 = 0;
		}

		if(y1 < 0) {
			y1 = 0;
		}

		int x2 = x1 + show_width;
		int y2 = y1 + show_height;
		if(x2 > image_texture.width()) {
			x1 -= (x2 - image_texture.width());
			x2 = image_texture.width();
			if(x1 < 0) {
				x1 = 0;
			}
		}

		if(y2 > image_texture.height()) {
			y1 -= (y2 - image_texture.height());
			y2 = image_texture.height();
			if(y1 < 0) {
				y1 = 0;
			}
		}

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
				bool rect_chosen = false;
				if(box.w() > 10 && box.h() > 10) {
					rect_chosen = point_in_rect(point(mousex, mousey), rect(box.x()+5, box.y()+5, box.w()-10, box.h()-10));
				}

				if(rect_chosen || point_in_rect(point(mousex, mousey), rect(box.x(), box.y()-4, box.w(), 9))) {
					dragging_sides_bitmap_ |= TOP_SIDE;
					graphics::draw_rect(rect(box.x(), box.y()-1, box.w(), 2).sdl_rect(), graphics::color_red());
				}
				
				if(rect_chosen || !(dragging_sides_bitmap_&TOP_SIDE) && point_in_rect(point(mousex, mousey), rect(box.x(), box.y2()-4, box.w(), 9))) {
					dragging_sides_bitmap_ |= BOTTOM_SIDE;
					graphics::draw_rect(rect(box.x(), box.y2()-1, box.w(), 2).sdl_rect(), graphics::color_red());
				}

				if(rect_chosen || point_in_rect(point(mousex, mousey), rect(box.x()-4, box.y(), 9, box.h()))) {
					dragging_sides_bitmap_ |= LEFT_SIDE;
					graphics::draw_rect(rect(box.x()-1, box.y(), 2, box.h()).sdl_rect(), graphics::color_red());
				}
				
				if(rect_chosen || (!dragging_sides_bitmap_&LEFT_SIDE) && point_in_rect(point(mousex, mousey), rect(box.x2()-4, box.y(), 9, box.h()))) {
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

	const int framex = preview_area.x() + (preview_area.w() - frame_->width()*scale)/2;
	const int framey = preview_area.y() + (preview_area.h() - frame_->height()*scale)/2;
	frame_->draw(framex, framey, true, false, cycle_, 0, scale);
	if(++cycle_ >= frame_->duration()) {
		cycle_ = 0;
	}

	solid_rect_ = rect();

	const_solid_info_ptr solid = frame_->solid();
	if(solid && solid->area().w()*solid->area().h()) {
		const rect area = solid->area();
		solid_rect_ = rect(framex + area.x(), framey + area.y(), area.w(), area.h());
		graphics::draw_rect(solid_rect_, graphics::color(255, 255, 255, 64));
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

	if(event.type == SDL_MOUSEBUTTONUP) {
		moving_solid_rect_ = false;
	}

	if(event.type == SDL_MOUSEMOTION) {
		has_motion_ = true;
		const SDL_MouseMotionEvent& e = event.motion;
		if(moving_solid_rect_) {
			if(solid_handler_) {
				const int x = e.x/2;
				const int y = e.y/2;

				solid_handler_(x - anchor_solid_x_, y - anchor_solid_y_);

				anchor_solid_x_ = x;
				anchor_solid_y_ = y;
			}

			return claimed;
		}

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
		moving_solid_rect_ = false;

		const SDL_MouseButtonEvent& e = event.button;
		const point p(e.x, e.y);
		anchor_area_ = frame_->area();
		anchor_pad_ = frame_->pad();
		has_motion_ = false;
		if(point_in_rect(p, dst_rect_)) {
			claimed = true;
			anchor_x_ = e.x;
			anchor_y_ = e.y;
		} else {
			anchor_x_ = anchor_y_ = -1;

			if(point_in_rect(p, solid_rect_)) {
				moving_solid_rect_ = true;
				anchor_solid_x_ = e.x/2;
				anchor_solid_y_ = e.y/2;
			}
		}
	} else if(event.type == SDL_MOUSEBUTTONUP && anchor_x_ != -1) {

		const SDL_MouseButtonEvent& e = event.button;
		point anchor(anchor_x_, anchor_y_);
		point p(e.x, e.y);
		if(anchor == p && !has_motion_) {
			claimed = true;
			p = mouse_point_to_image_loc(p);
			graphics::surface surf = graphics::surface_cache::get(obj_["image"].as_string());
			std::vector<graphics::surface> surf_key;
			surf_key.push_back(surf);
			if(surf) {
				surf = graphics::texture::build_surface_from_key(surf_key, surf->w, surf->h);
			}

			if(surf) {
				rect area = get_border_rect_around_loc(surf, p.x, p.y);
				if(area.w() > 0) {
					if(rect_handler_) {
						rect_handler_(area);
					}

					int pad = frame_->pad();
					int num_frames = 1;
					int frames_per_row = 1;
					if(find_full_animation(surf, area, &pad, &num_frames, &frames_per_row)) {
						if(pad_handler_) {
							pad_handler_(pad);
						}

						if(num_frames_handler_) {
							std::cerr << "SETTING NUM FRAMES TO " << num_frames << "\n";
							num_frames_handler_(num_frames);
						}

						if(frames_per_row_handler_) {
							frames_per_row_handler_(frames_per_row);
						}
					}
				}
			}
		} else if(!dragging_sides_bitmap_ && point_in_rect(anchor, dst_rect_) && point_in_rect(p, dst_rect_)) {
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

void animation_preview_widget::reset_rect()
{
	if(rect_handler_) {
		rect_handler_(rect(0,0,0,0));
	}
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

void animation_preview_widget::set_num_frames_handler(boost::function<void(int)> handler)
{
	num_frames_handler_ = handler;
}

void animation_preview_widget::set_frames_per_row_handler(boost::function<void(int)> handler)
{
	frames_per_row_handler_ = handler;
}

void animation_preview_widget::set_solid_handler(boost::function<void(int,int)> handler)
{
	solid_handler_ = handler;
}

void animation_preview_widget::rect_handler_delegate(rect r)
{
	using namespace game_logic;
	if(get_environment()) {
		map_formula_callable_ptr callable = map_formula_callable_ptr(new map_formula_callable(get_environment()));
		callable->add("new_rect", r.write());
		variant value = ffl_rect_handler_->execute(*callable);
		get_environment()->execute_command(value);
	} else {
		std::cerr << "animation_preview_widget::rect_handler_delegate() called without environment!" << std::endl;
	}
}

void animation_preview_widget::pad_handler_delegate(int pad)
{
	using namespace game_logic;
	if(get_environment()) {
		map_formula_callable_ptr callable = map_formula_callable_ptr(new map_formula_callable(get_environment()));
		callable->add("new_pad", variant(pad));
		variant value = ffl_pad_handler_->execute(*callable);
		get_environment()->execute_command(value);
	} else {
		std::cerr << "animation_preview_widget::pad_handler_delegate() called without environment!" << std::endl;
	}
}

void animation_preview_widget::num_frames_handler_delegate(int frames)
{
	using namespace game_logic;
	if(get_environment()) {
		map_formula_callable_ptr callable = map_formula_callable_ptr(new map_formula_callable(get_environment()));
		callable->add("new_frames", variant(frames));
		variant value = ffl_num_frames_handler_->execute(*callable);
		get_environment()->execute_command(value);
	} else {
		std::cerr << "animation_preview_widget::num_frames_handler_delegate() called without environment!" << std::endl;
	}
}

void animation_preview_widget::frames_per_row_handler_delegate(int frames_per_row)
{
	using namespace game_logic;
	if(get_environment()) {
		map_formula_callable_ptr callable = map_formula_callable_ptr(new map_formula_callable(get_environment()));
		callable->add("new_frames_per_row", variant(frames_per_row));
		variant value = ffl_frames_per_row_handler_->execute(*callable);
		get_environment()->execute_command(value);
	} else {
		std::cerr << "animation_preview_widget::frames_per_row_handler_delegate() called without environment!" << std::endl;
	}
}

void animation_preview_widget::solid_handler_delegate(int x, int y)
{
	using namespace game_logic;
	if(get_environment()) {
		map_formula_callable_ptr callable = map_formula_callable_ptr(new map_formula_callable(get_environment()));
		callable->add("new_solidx", variant(x));
		callable->add("new_solidy", variant(y));
		variant value = ffl_solid_handler_->execute(*callable);
		get_environment()->execute_command(value);
	} else {
		std::cerr << "animation_preview_widget::solid_handler_delegate() called without environment!" << std::endl;
	}
}

void animation_preview_widget::set_value(const std::string& key, const variant& v)
{
	if(key == "object") {
		try {
			set_object(v);
		} catch(type_error&) {
		} catch(frame::error&) {
		} catch(validation_failure_exception&) {
		} catch(graphics::load_image_error&) {
		}
	}
	widget::set_value(key, v);
}

variant animation_preview_widget::get_value(const std::string& key) const
{
	if(key == "object") {
		return obj_;
	}
	return widget::get_value(key);
}

}

#endif // !NO_EDITOR

