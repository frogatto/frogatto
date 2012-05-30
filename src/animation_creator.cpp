#include <sstream>
#include <math.h>

#include "animation_creator.hpp"
#include "draw_scene.hpp"
#include "file_chooser_dialog.hpp"
#include "foreach.hpp"
#include "frame.hpp"
#include "graphics.hpp"
#include "level.hpp"
#include "module.hpp"
#include "preferences.hpp"
#include "raster.hpp"
#include "slider.hpp"
 
namespace gui {

namespace {

void do_draw_scene() {
	draw_scene(level::current(), last_draw_position());
}

int slider_transform(double d)
{
	// normalize to [-20.0,20.0] range.
	d = (d - 0.5) * 2.0 * 20;
	double d_abs = abs(d);
	if(d_abs > 10) {
		// Above 10 units we go non-linear.
		return int((d < 0 ? -1.0 : 1.0) * pow(10, d_abs/10));
	}
	return int(d);
}

}

animation_creator_dialog::animation_creator_dialog(int x, int y, int w, int h, const variant& anims)
	: dialog(x,y,w,h), selected_frame_(-1), cycle_(0), anims_(anims), dragging_slider_(false),
	reverse_(false), rotate_on_slope_(false)
{
	set_clear_bg_amount(255);
	if(anims.is_list()) {
		foreach(const variant& v, anims.as_list()) {
			frames_.push_back(boost::shared_ptr<frame>(new frame(v)));
		}

		if(frames_.empty() == false) {
			selected_frame_ = 0;
			set_default_properties();
			set_properties();
		}
	}	
	preview_area_ = rect(width()-128, height()-128, 128, 128);

	init();
}

void animation_creator_dialog::set_default_properties()
{
	int_properties_["frames"] = 1;
	int_properties_["frames_per_row"] = -1;
	int_properties_["duration"] = -1;
	int_properties_["pad"] = 0;
	int_properties_["rotate"] = 0;
	int_properties_["blur"] = 0;
	int_properties_["damage"] = 0;
	int_properties_["feet_x"] = 0;
	int_properties_["feet_y"] = 0;
	int_properties_["velocity_x"] = INT_MIN;
	int_properties_["velocity_y"] = INT_MIN;
	int_properties_["accel_x"] = INT_MIN;
	int_properties_["accel_y"] = INT_MIN;
	int_properties_["scale"] = 2;
}

void animation_creator_dialog::set_properties()
{
	variant v = anims_.as_list()[selected_frame_];
	variant keys = v.get_keys();
	foreach(const variant& key, keys.as_list()) {
		std::string s = key.as_string();
		if(v[s].is_int()) {
			int_properties_[s] = v[s].as_int();
		}
	}
}


void animation_creator_dialog::init()
{
	const int x_offset = 35;
	int current_height = 35;
	int hpad = 10;

	clear();

	// Add copy desintation box
	grid_ptr g(new grid(2));
	g->set_hpad(20);
	label_ptr copy_dest_label(new label(module::get_module_path("") + "images/", 14));
	g->add_col(button_ptr(new button(new label("Set Destination", 14), boost::bind(&animation_creator_dialog::set_destination, this, copy_dest_label))));
	copy_dest_label->set_color(graphics::color_green());
	g->add_col(copy_dest_label);
	add_widget(g, x_offset, current_height);
	current_height += g->height() + hpad;

	// Add current list of animations
	g.reset(new grid(3));
	g->set_dim(int(width() * 0.8), int(height()/5));
	g->set_max_height(int(height()/5));
	g->set_show_background(true);
	g->set_hpad(10);
	g->allow_selection(true);
	g->add_col(label_ptr(new label("Identifier", 14)))
		.add_col(label_ptr(new label("Image Path", 14)))
		.add_col(label_ptr(new label("Area in Image", 14)));
	foreach(const boost::shared_ptr<frame>& f, frames_) {
		std::stringstream ss;
		ss << f->area();
		g->add_col(label_ptr(new label(f->id(), 12)))
			.add_col(label_ptr(new label(f->image_name(), 12)))
			.add_col(label_ptr(new label(ss.str(), 12)));
	}
	g->register_selection_callback(boost::bind(&animation_creator_dialog::select_animation, this, _1));
	add_widget(g, (width() - g->width() - x_offset)/2, current_height);
	current_height += g->height() + hpad;

	// Animation fields
	// id
	// image file
	// image rect
	// frames, frames per row, duration
	// pad, rotate, blur
	// reverse (bool), rotate_on_slope(bool)
	// damage
	// collide rect
	// hit ret
	// platform rect
	// feet_x, feet_y
	// velocity_x, velocity_y
	// accel_x, accel_y
	g.reset(new grid(3));
	g->set_max_height(int(height()/2));
	text_editor_widget_ptr id_entry(new text_editor_widget(200, 28));
	id_entry->set_font_size(14);
	id_entry->set_text(id_);
	id_entry->set_on_change_handler(boost::bind(&animation_creator_dialog::on_id_change, this));
	g->add_col(widget_ptr(new label("Identifier: ", graphics::color_white(), 14)))
		.add_col(widget_ptr(id_entry))
		.finish_row();

	g->add_col(button_ptr(new button(new label("Choose Image File", 14), boost::bind(&animation_creator_dialog::set_image_file, this))))
		.add_col(widget_ptr(new label(image_file_, graphics::color_green(), 14)))
		.add_col(widget_ptr(new label("Will be copied to the destination directory", graphics::color_yellow(), 14)));

	std::pair<std::string, int> p;
	foreach(p, int_properties_) {
		text_editor_widget_ptr entry(new text_editor_widget(100, 28));
		std::stringstream ss;
		ss << int_properties_[p.first];
		entry->set_text(ss.str());
		slider_ptr slide(new slider(200, boost::bind((&animation_creator_dialog::change_slide), this, p.first, entry, _1), 0.5));
		slide->set_drag_end(boost::bind(&animation_creator_dialog::end_slide, this, p.first, slide, entry, _1));
		entry->set_on_change_handler(boost::bind(&animation_creator_dialog::change_text, this, p.first, entry, slide));
		slider_offset_[p.first] = p.second;
		g->add_col(widget_ptr(new label(p.first, graphics::color_white(), 12)))
			.add_col(entry)
			.add_col(slide);
	}
	add_widget(g, x_offset, current_height);
	current_height += g->height() + hpad;

	// Add/Delete animation buttons
	g.reset(new grid(3));
	g->set_hpad(40);
	g->add_col(button_ptr(new button(new label("New", 14), boost::bind(&animation_creator_dialog::anim_new, this))))
		.add_col(button_ptr(new button(new label("Add", 14), boost::bind(&animation_creator_dialog::anim_add, this))))
		.add_col(button_ptr(new button(new label("Delete", 14), boost::bind(&animation_creator_dialog::anim_del, this))));
	add_widget(g, x_offset, current_height);
	current_height += g->height() + hpad;
}

void animation_creator_dialog::handle_draw() const
{
	dialog::handle_draw();

	if(selected_frame_ >= 0) {
		ASSERT_LT(size_t(selected_frame_), frames_.size());
		GLfloat w = GLfloat(frames_[selected_frame_]->width());
		GLfloat h = GLfloat(frames_[selected_frame_]->height());
		GLfloat scale = std::min(GLfloat(preview_area_.w()/w), GLfloat(preview_area_.h()/h));

		const int framex = preview_area_.x();
		const int framey = preview_area_.y();
		frames_[selected_frame_]->draw(framex, framey, true, false, cycle_, 0, scale);
		if(++cycle_ >= frames_[selected_frame_]->duration()) {
			cycle_ = 0;
		}
	}
}

void animation_creator_dialog::on_id_change()
{
}

void animation_creator_dialog::set_image_file()
{
}

void animation_creator_dialog::change_text(const std::string& s, text_editor_widget_ptr editor, slider_ptr slide)
{
	if(!dragging_slider_) {
		int i;
		std::istringstream(editor->text()) >> i;
		slider_offset_[s] = i;
		slide->set_position(0.5);
		int_properties_[s] = i;
	}
}

void animation_creator_dialog::change_slide(const std::string& s, text_editor_widget_ptr editor, double d)
{
	dragging_slider_ = true;
	std::ostringstream ss;
	int i = slider_transform(d) + slider_offset_[s];
	ss << i;
	editor->set_text(ss.str());
	int_properties_[s] = i;
}

void animation_creator_dialog::end_slide(const std::string& s, slider_ptr slide, text_editor_widget_ptr editor, double d)
{
	int i = slider_transform(d) + slider_offset_[s];
	slider_offset_[s] = i;
	slide->set_position(0.5);
	dragging_slider_ = false;
}

void animation_creator_dialog::anim_add()
{
}

void animation_creator_dialog::anim_del()
{
}

void animation_creator_dialog::anim_new()
{
}

void animation_creator_dialog::select_animation(int index)
{
	// index 0 is our label row.
	if(index < 1 || size_t(index) > frames_.size()) {
		return;
	}
	selected_frame_ = index - 1;
	set_properties();
	cycle_ = 0;
	init();
}

void animation_creator_dialog::set_destination(label_ptr copy_dest)
{
	file_chooser_dialog dir_dlg(
		int(preferences::virtual_screen_width()*0.2), 
		int(preferences::virtual_screen_height()*0.2), 
		int(preferences::virtual_screen_width()*0.6), 
		int(preferences::virtual_screen_height()*0.6),
		gui::filter_list(), 
		true, module::get_module_path("") + "images/");
	dir_dlg.set_background_frame("empty_window");
	dir_dlg.set_draw_background_fn(do_draw_scene);
	dir_dlg.use_relative_paths(true);
	dir_dlg.show_modal();

	if(dir_dlg.cancelled() == false) {
		copy_dest->set_text(dir_dlg.get_path());
	}
	init();
}

}
