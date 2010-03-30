#include <iostream>

#include <boost/bind.hpp>
#include <boost/function.hpp>

#include <math.h>

#include "asserts.hpp"
#include "button.hpp"
#include "custom_object_type.hpp"
#include "dialog.hpp"
#include "filesystem.hpp"
#include "foreach.hpp"
#include "formatter.hpp"
#include "frame.hpp"
#include "geometry.hpp"
#include "grid_widget.hpp"
#include "label.hpp"
#include "raster.hpp"
#include "surface.hpp"
#include "surface_cache.hpp"
#include "text_entry_widget.hpp"
#include "texture.hpp"
#include "unit_test.hpp"
#include "wml_parser.hpp"
#include "wml_utils.hpp"
#include "wml_writer.hpp"

namespace {
using namespace gui;
using graphics::surface;

const unsigned char RedBorder[] = {0xf9, 0x30, 0x3d};

bool is_pixel_border(const surface& s, int x, int y)
{
	if(x < 0 || y < 0 || x >= s->w || y >= s->h) {
		return false;
	}

	unsigned char* pixel = reinterpret_cast<unsigned char*>(s->pixels) + y*s->pitch + x*3;
	for(int n = 0; n != 3; ++n) {
		if(pixel[n] != RedBorder[n]) {
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

typedef std::vector<std::vector<rect> > RectSelection;

class object_image_widget : public widget
{
	boost::function<void(RectSelection)> selection_handler_;
public:
	object_image_widget(const std::string& fname, boost::function<void(RectSelection)> handler) : selection_handler_(handler)
	{
		surface_ = graphics::surface_cache::get(fname);
		ASSERT_LOG(surface_->format->Rmask == 0xFF && surface_->format->Gmask == 0xFF00 && surface_->format->Bmask == 0xFF0000 && surface_->format->Amask == 0, "SURFACE NOT IN EXPECTED FORMAT");

		for(int y = 0; y != surface_->h; ++y) {
			for(int x = 0; x != surface_->w; ++x) {
				if(is_pixel_border(surface_, x, y) &&
				  !is_pixel_border(surface_, x-1, y) &&
				  !is_pixel_border(surface_, x, y-1)) {
					rect r = get_border_rect(surface_, x, y);
					if(r.w()*r.h() != 0) {
						rects_.push_back(r);
					}
				}
			}
		}

		std::vector<surface> surfs;
		surfs.push_back(surface_);
		texture_ = graphics::texture(surfs);

		set_dim(texture_.width()*2, texture_.height()*2);
	}

	void handle_draw() const {
		int mousex, mousey;
		const unsigned int buttons = SDL_GetMouseState(&mousex, &mousey);
		mousex -= x();
		mousey -= y();
		point mouse_pos(mousex/2, mousey/2);

		const float wave = sin(SDL_GetTicks()/200.0);

		graphics::blit_texture(texture_, x(), y(), width(), height());
		foreach(const rect& r, rects_) {
			graphics::draw_rect(rect(x() + r.x()*2, y() + r.y()*2, r.w()*2, r.h()*2), point_in_rect(mouse_pos, r) ? graphics::color(255, 255, 0, 64 + wave*64.0) : graphics::color(255, 255, 255, 64));
		}

		foreach(const std::vector<rect>& row, selected_rects_) {
			foreach(const rect& r, row) {
				graphics::draw_rect(rect(x() + r.x()*2, y() + r.y()*2, r.w()*2, r.h()*2), graphics::color(255, 255, 0, 64 + wave*64.0));
			}
		}
	}

	std::vector<std::vector<rect> > calculate_selected_rects() const {
		int mousex, mousey;
		const unsigned int buttons = SDL_GetMouseState(&mousex, &mousey);
		mousex -= x();
		mousey -= y();

		point mouse_pos(mousex/2, mousey/2);
		if(buttons) {
			return get_selected_rects(rect::from_coordinates(mouse_pos.x, mouse_pos.y, anchor_.x, anchor_.y));
		} else {
			return std::vector<std::vector<rect> >();
		}
	}

	bool handle_event(const SDL_Event& event, bool claimed)
	{
		if(event.type == SDL_MOUSEBUTTONDOWN) {
			int mousex, mousey;
			const unsigned int buttons = SDL_GetMouseState(&mousex, &mousey);
			if(mousex >= x() && mousex <= x() + width() && mousey >= y() && mousey <= y() + height()) {
				mousex -= x();
				mousey -= y();
				mousex /= 2;
				mousey /= 2;

				anchor_ = point(mousex, mousey);
			}
		}

		if(event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEMOTION) {
			int mousex, mousey;
			const unsigned int buttons = SDL_GetMouseState(&mousex, &mousey);
			if((buttons || event.type == SDL_MOUSEBUTTONDOWN) &&
			   mousex >= x() && mousex <= x() + width() &&
			   mousey >= y() && mousey <= y() + height()) {
				std::vector<std::vector<rect> > selection = calculate_selected_rects();
				if(selection != selected_rects_ && !selection.empty()) {
					selected_rects_ = selection;
					selection_handler_(selected_rects_);
				}
			}
		}

		return false;
	}

	const rect* rect_at_point(const point& p) const
	{
		foreach(const rect& r, rects_) {
			if(point_in_rect(p, r)) {
				return &r;
			}
		}

		return NULL;
	}

	std::vector<std::vector<rect> > get_selected_rects(const rect& selection) const
	{
		std::vector<std::vector<rect> > result;
		const rect* first_rect = rect_at_point(selection.top_left());
		const rect* last_rect = rect_at_point(selection.bottom_right());
		if(!first_rect || !last_rect) {
			return result;
		}

		std::vector<rect> rects;
		foreach(const rect& r, rects_) {
			if(r.x() >= first_rect->x() && r.y() >= first_rect->y() &&
			   r.x2() <= last_rect->x2() && r.y2() <= last_rect->y2()) {
				rects.push_back(r);
			}
		}

		if(rects.empty()) {
			return result;
		}
		
		const int rect_width = rects.front().w();
		const int rect_height = rects.front().h();

		foreach(const rect& r, rects) {
			if(r.w() != rect_width && r.h() != rect_height) {
				return result;
			}
		}

		int pad = -1;
		int n;
		for(n = 1; n < rects.size(); ++n) {
			const int new_pad = rects[n].x() - rects[n-1].x2();
			if(pad != -1 && new_pad != pad) {
				break;
			}

			if(rects[n].y() != rects[n-1].y() || rects[n].y2() != rects[n-1].y2()) {
				break;
			}

			pad = new_pad;
		}

		const int row_length = n;

		if(n < rects.size()) {
			if((rects.size()%n) != 0) {
				return result;
			}

			for(; n < rects.size(); ++n) {
				const int new_pad = rects[n].y() - rects[n-row_length].y2();
				if(pad != -1 && new_pad != pad) {
					return result;
				}

				if(rects[n].x() != rects[n-row_length].x()) {
					return result;
				}

				pad = new_pad;
			}
		}

		for(n = 0; n < rects.size(); n += row_length) {
			ASSERT_LE(n+row_length, rects.size());
			result.push_back(std::vector<rect>(rects.begin() + n, rects.begin() + n + row_length));
		}

		return result;
	}

	const point& anchor() const { return anchor_; }

	void set_selection(const RectSelection& selection) { selected_rects_ = selection; }

private:
	graphics::texture texture_;
	surface surface_;
	std::vector<rect> rects_;

	std::vector<std::vector<rect> > selected_rects_;

	point anchor_;
};

class object_image_dialog : public gui::dialog
{
public:
	object_image_dialog(const std::string& image_name, wml::node_ptr node)
	  : dialog(0, 0, 800, 600), image_name_(image_name), node_(node),
	    duration_(1), reverse_(false)
	{
		for(wml::node::child_iterator i = node->begin_child("animation"); i != node->end_child("animation"); ++i) {
			wml::node_ptr anim_node = i->second;
			anim_nodes_.insert(std::pair<std::string, wml::node_ptr>(anim_node->attr("id"), anim_node));
		}

		wml::node_ptr anim = node->get_child("animation");
		ASSERT_LOG(anim.get() != NULL, "OBJECT HAS NO ANIMATIONS");
		edit_animation(anim);
	}

	void set_selection(RectSelection selection) {
		if(selection.empty() || selection.front().empty() || !animation_editing_) {
			return;
		}

		const rect& first_rect = selection.front().front();

		const rect rect_scaled(first_rect.x()/2, first_rect.y()/2, first_rect.w()/2, first_rect.h()/2);

		int pad = 0;
		if(selection.front().size() > 1) {
			pad = selection.front()[1].x() - selection.front()[0].x2();
		} else if(selection.size() > 1) {
			pad = selection[1].front().y() - selection[0].front().y2();
		}

		animation_editing_->set_attr("rect", first_rect.to_string());
		animation_editing_->set_attr("frames", formatter() << selection.size()*selection.front().size());
		animation_editing_->set_attr("frames_per_row", formatter() << selection.front().size());
		animation_editing_->set_attr("pad", formatter() << pad);

		frame_.reset(new frame(animation_editing_));
	}

	void handle_draw() const {
		dialog::handle_draw();

		if(!frame_) {
			return;
		}

		static int nframe = 0;

		const int time_in_frame = nframe%frame_->duration();
		frame_->draw(600, 460, true, false, time_in_frame);
		++nframe;
	}
	
	void init() {
		clear();

		grid_ptr g(new grid(2));
		g->set_hpad(10);
		g->add_col(widget_ptr(new button(widget_ptr(new label("Ok", graphics::color_white())), boost::bind(&dialog::close, this))));
		g->add_col(widget_ptr(new button(widget_ptr(new label("Cancel", graphics::color_white())), boost::bind(&dialog::cancel, this))));

		add_widget(g, 660, 560);

		object_image_widget* image_widget = new object_image_widget(image_name_, boost::bind(&object_image_dialog::set_selection, this, _1));

		if(frame_) {
			std::vector<rect> rects;
			rects.push_back(frame_->area());
			for(int n = 1; n < frame_->num_frames() && n < frame_->num_frames_per_row(); ++n) {
				rects.push_back(rect(rects.back().x2() + frame_->pad(), rects.back().y(), rects.back().w(), rects.back().h()));
			}

			for(int n = frame_->num_frames_per_row(); n < frame_->num_frames(); ++n) {
				const rect& above = rects[n - frame_->num_frames_per_row()];
				rects.push_back(rect(above.x(), above.y2() + frame_->pad(), above.w(), above.h()));
			}

			RectSelection selection;
			for(int n = 0; n < frame_->num_frames(); n += frame_->num_frames_per_row()) {
				ASSERT_LE(n, frame_->num_frames() - frame_->num_frames_per_row());
				std::vector<rect> v(rects.begin() + n, rects.begin() + n + frame_->num_frames_per_row());
				selection.push_back(v);
			}

			image_widget->set_selection(selection);
		}

		add_widget(widget_ptr(image_widget), 0, 0);

		animation_id_label_.reset(new label(animation_editing_->attr("id"), graphics::color_white()));

		add_widget(widget_ptr(new label(node_->attr("id"), graphics::color_white(), 16)), 580, 2);

		g = grid_ptr(new grid(2));
		g->set_hpad(10);
		g->add_col(animation_id_label_);
		g->add_col(widget_ptr(new button(widget_ptr(new label("Change Name", graphics::color_white())), boost::bind(&object_image_dialog::change_name, this))));

		add_widget(g);

		g = grid_ptr(new grid(4));
		g->set_hpad(10);
		g->add_col(widget_ptr(new label("Duration:", graphics::color_white())));
		g->add_col(widget_ptr(new label(formatter() << duration_, graphics::color_white())));
		g->add_col(widget_ptr(new button(widget_ptr(new label("+", graphics::color_white())), boost::bind(&object_image_dialog::change_duration, this, 1))));
		g->add_col(widget_ptr(new button(widget_ptr(new label("-", graphics::color_white())), boost::bind(&object_image_dialog::change_duration, this, -1))));
		add_widget(g);

		g = grid_ptr(new grid(3));
		g->set_hpad(10);
		g->add_col(widget_ptr(new label("Reverse:", graphics::color_white())));
		g->add_col(widget_ptr(new label(reverse_ ? "yes" : "no", graphics::color_white())));
		g->add_col(widget_ptr(new button(widget_ptr(new label("Toggle", graphics::color_white())), boost::bind(&object_image_dialog::toggle_reverse, this))));
		add_widget(g);

		g = grid_ptr(new grid(3));
		for(std::multimap<std::string, wml::node_ptr>::iterator i = anim_nodes_.begin(); i != anim_nodes_.end(); ++i) {
			g->add_col(widget_ptr(new label(i->first, i->second == animation_editing_ ? graphics::color_yellow() : graphics::color_white())));
			g->add_col(widget_ptr(new button(widget_ptr(new label("Edit", graphics::color_white())), boost::bind(&object_image_dialog::edit_animation, this, i->second))));
			if(anim_nodes_.size() > 1) {
				g->add_col(widget_ptr(new button(widget_ptr(new label("Delete", graphics::color_white())), boost::bind(&object_image_dialog::delete_animation, this, i->second))));
			} else {
				g->finish_row();
			}
		}

		add_widget(g);

		add_widget(widget_ptr(new button(widget_ptr(new label("New animation", graphics::color_white())), boost::bind(&object_image_dialog::new_animation, this))));
	}

private:
	void new_animation() {
		create_new_animation();
		init();
	}

	void edit_animation(wml::node_ptr node) {
		animation_editing_ = node;
		duration_ = wml::get_int(animation_editing_, "duration", 1);
		reverse_ = wml::get_bool(animation_editing_, "reverse", false);
		frame_.reset(new frame(animation_editing_));
		init();
	}

	void delete_animation(wml::node_ptr node) {
		node_->erase_child(node);
		if(animation_editing_ == node) {
			edit_animation(node_->get_child("animation"));
		}

		for(std::multimap<std::string, wml::node_ptr>::iterator i = anim_nodes_.begin(); i != anim_nodes_.end(); ++i) {
			if(i->second == node) {
				anim_nodes_.erase(i);
				break;
			}
		}

		init();
	}

	void create_new_animation() {
		animation_editing_ = wml::deep_copy(node_->get_child("animation"));
		duration_ = wml::get_int(animation_editing_, "duration", 1);
		animation_editing_->set_attr("duration", formatter() << duration_);

		reverse_ = wml::get_bool(animation_editing_, "reverse", false);

		node_->add_child(animation_editing_);
		anim_nodes_.insert(std::pair<std::string, wml::node_ptr>(animation_editing_->attr("id"), animation_editing_));
	}

	void change_duration(int delta) {
		duration_ += delta;
		if(duration_ < 1) {
			duration_ = 1;
		}

		animation_editing_->set_attr("duration", formatter() << duration_);
		frame_.reset(new frame(animation_editing_));
		init();
	}

	void toggle_reverse() {
		reverse_ = !reverse_;
		animation_editing_->set_attr("reverse", reverse_ ? "yes" : "no");
		frame_.reset(new frame(animation_editing_));
		init();
	}

	void change_name() {
		dialog d(0, 0, graphics::screen_width(), graphics::screen_height());
		text_entry_widget* entry = new text_entry_widget;
		d.add_widget(widget_ptr(new label("Name", graphics::color_white())))
		 .add_widget(widget_ptr(entry));
		d.show_modal();
		std::string name = entry->text();
		if(name.empty() == false) {
			animation_editing_->set_attr("id", name);
			for(std::multimap<std::string, wml::node_ptr>::iterator itor = anim_nodes_.begin(); itor != anim_nodes_.end(); ++itor) {
				if(itor->second == animation_editing_) {
					anim_nodes_.insert(std::pair<std::string, wml::node_ptr>(name, animation_editing_));
					anim_nodes_.erase(itor);
					break;
				}
			}
			init();
		}
	}

	std::string image_name_;
	wml::node_ptr node_;
	std::multimap<std::string, wml::node_ptr> anim_nodes_;

	wml::node_ptr animation_editing_;
	boost::shared_ptr<label> animation_id_label_;

	boost::shared_ptr<frame> frame_;
	int duration_;
	bool reverse_;
};

}

void show_object_editor_dialog(const std::string& obj_type)
{
	const std::string* fname = custom_object_type::get_object_path(obj_type + ".cfg");
	if(fname == NULL) {
		std::cerr << "OBJECT NOT FOUND\n";
	}

	const_custom_object_type_ptr type = custom_object_type::get(obj_type);
	ASSERT_LOG(type.get() != NULL, "COULD NOT LOAD OBJECT");

	wml::node_ptr obj_node = wml::parse_wml_from_file(*fname);
	wml::node_ptr node = custom_object_type::merge_prototype(obj_node);
	wml::const_node_ptr anim = node->get_child("animation");
	ASSERT_LOG(anim.get() != NULL, "COULD NOT FIND ANIMATION");

	const std::string image = anim->attr("image");

	object_image_dialog d(image, node);
	d.show_modal();
	if(d.cancelled()) {
		return;
	}

	if(obj_node != node) {
		while(obj_node->get_child("animation")) {
			obj_node->erase_child(obj_node->get_child("animation"));
		}

		FOREACH_WML_CHILD(anim_node, node, "animation") {
			obj_node->add_child(wml::deep_copy(anim_node));
		}
	}

	sys::write_file(*fname, wml::output(obj_node));
}

UTILITY(object_editor)
{
	ASSERT_LOG(args.empty() == false, "MUST SPECIFY NAME OF OBJECT TO EDIT");

	show_object_editor_dialog(args.front());
}
