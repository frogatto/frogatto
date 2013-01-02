#ifndef NO_EDITOR
#include <sstream>
#include <fstream>
#include <math.h>

#include "animation_creator.hpp"
#include "draw_scene.hpp"
#include "dropdown_widget.hpp"
#include "file_chooser_dialog.hpp"
#include "foreach.hpp"
#include "frame.hpp"
#include "graphics.hpp"
#include "level.hpp"
#include "message_dialog.hpp"
#include "module.hpp"
#include "preferences.hpp"
#include "raster.hpp"
#include "slider.hpp"
#include "surface_cache.hpp"
 
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

// Given two variants, which are maps merge the properties from v2 into v1 that
// don't already exist in v1.
void variant_map_merge(variant& v1, const variant& v2)
{
	std::map<variant, variant>::const_iterator v2it = v2.as_map().begin();
	std::map<variant, variant>::const_iterator v2end = v2.as_map().end();
	while(v2it != v2end) {
		std::map<variant, variant>::const_iterator v1it = v1.as_map().find(v2it->first);
		if(v1it == v1.as_map().end()) {
			v1.add_attr(v2it->first, v2it->second);
		}
		v2it++;
	}
}

void load_default_properties(std::map<variant, variant>* def_properties)
{
	// integer properties
	(*def_properties)[variant("frames")] = variant(1);
	(*def_properties)[variant("frames_per_row")] = variant(-1);
	(*def_properties)[variant("duration")] = variant(-1);
	(*def_properties)[variant("pad")] = variant(0);
	(*def_properties)[variant("rotate")] = variant(0);
	(*def_properties)[variant("blur")] = variant(0);
	(*def_properties)[variant("damage")] = variant(0);
	(*def_properties)[variant("feet_x")] = variant(0);
	(*def_properties)[variant("feet_y")] = variant(0);
	(*def_properties)[variant("velocity_x")] = variant(INT_MIN);
	(*def_properties)[variant("velocity_y")] = variant(INT_MIN);
	(*def_properties)[variant("accel_x")] = variant(INT_MIN);
	(*def_properties)[variant("accel_y")] = variant(INT_MIN);
	(*def_properties)[variant("scale")] = variant(2);
		
	(*def_properties)[variant("id")] = variant("id");
	//(*def_properties)[variant("image")] = variant("");

	std::vector<variant> v;
	//rects
	(*def_properties)[variant("rect")] = variant(&v);
	(*def_properties)[variant("collide")] = variant(&v);
	(*def_properties)[variant("hit")] = variant(&v);
	(*def_properties)[variant("platform")] = variant(&v);

	// some bools
	(*def_properties)[variant("reverse")] = variant::from_bool(false);
	(*def_properties)[variant("play_backwards")] = variant::from_bool(false);
	(*def_properties)[variant("rotate_on_slope")] = variant::from_bool(false);
}

std::map<variant, variant>& get_default_properties()
{
	static std::map<variant, variant> defs;
	if(defs.empty()) {
		load_default_properties(&defs);
	}
	return defs;
}

}


animation_creator_dialog::animation_creator_dialog(int x, int y, int w, int h, const variant& anims)
	: dialog(x,y,w,h), selected_frame_(-1), dragging_slider_(false), changed_(false), simple_options_(true)
{
	set_clear_bg_amount(255);
	reset_current_object();
	if(anims.is_list()) {
		anims_ = anims.as_list();
		if(anims_.size() > 0) {
			selected_frame_ = 0;
			current_ = anims_[selected_frame_];
		}
	} else if(anims.is_map() && anims.has_key("image")){
		anims_.push_back(anims);
	}

	set_process_hook(boost::bind(&animation_creator_dialog::process, this));
	init();
}

std::vector<std::string> animation_creator_dialog::common_animation_list()
{
	// List of common animations.
	std::vector<std::string> v;
	v.push_back("stand");
	v.push_back("normal");
	v.push_back("hurt");
	v.push_back("turn");
	v.push_back("walk");
	v.push_back("spring");
	v.push_back("fly");
	v.push_back("jump");
	v.push_back("fall");
	v.push_back("open");
	v.push_back("ajar");
	v.push_back("close");
	v.push_back("land");
	v.push_back("thrown");
	v.push_back("lose_wings");
	v.push_back("portrait");
	v.push_back("swim");
	v.push_back("attack");
	v.push_back("cling");
	v.push_back("fire");
	v.push_back("jump_attack");
	v.push_back("run");
	v.push_back("crouch");
	v.push_back("enter_crouch");
	v.push_back("enter_lookup");
	v.push_back("flash");
	v.push_back("leave_crouch");
	v.push_back("lookup");
	v.push_back("pushed");
	v.push_back("roll");
	v.push_back("run_attack");
	v.push_back("shoot");
	return v;
}

void animation_creator_dialog::init()
{
	const int border_offset = 35;
	int current_height = 35;
	int hpad = 10;

	clear();

	// Add copy desintation box
	grid_ptr g(new grid(2));
	g->set_hpad(20);
	g->add_col(button_ptr(new button(new label("Set Destination", 14), boost::bind(&animation_creator_dialog::set_destination, this))))
		.add_col(label_ptr(new label(copy_path_, graphics::color_green(), 14)));
	g->add_col(widget_ptr(new label("", graphics::color_yellow(), 12)))
		.add_col(widget_ptr(new label("Images will be copied to the destination directory", graphics::color_yellow(), 12)));
	add_widget(g, border_offset, current_height);
	current_height += g->height() + hpad;

	// Add current list of animations
	g.reset(new grid(3));
	g->set_dim(width()/2, height()/5);
	g->set_max_height(height()/5);
	g->set_show_background(true);
	g->set_hpad(10);
	g->set_header_row(0);
	g->allow_selection(true);
	g->add_col(label_ptr(new label("Identifier", 14)))
		.add_col(label_ptr(new label("Image Path", 14)))
		.add_col(label_ptr(new label("Area in Image", 14)));
	foreach(const variant& v, anims_) {
		std::stringstream ss;
		rect r;
		if(v.has_key("rect")) {
			r = rect(v["rect"]);
		} else if(v.has_key("x") && v.has_key("y") && v.has_key("w") && v.has_key("h")) {
			r = rect(v["x"].as_int(),
	                v["y"].as_int(),
	                v["w"].as_int(),
	                v["h"].as_int());
		}
		ss << r;
		g->add_col(label_ptr(new label(v.has_key("id") ? v["id"].as_string() : "<missing>", 12)))
			.add_col(label_ptr(new label(v.has_key("image") ? v["image"].as_string() : "", 12)))
			.add_col(label_ptr(new label(ss.str(), 12)));
	}
	g->register_selection_callback(boost::bind(&animation_creator_dialog::select_animation, this, _1));
	add_widget(g, border_offset, current_height);
	current_height += g->height() + hpad;

	g.reset(new grid(3));
	g->set_max_height(int(height()/2 - 50));
	g->set_zorder(1);

	dropdown_widget_ptr id_entry(new dropdown_widget(common_animation_list(), 150, 28, dropdown_widget::DROPDOWN_COMBOBOX));
	id_entry->set_font_size(14);
	id_entry->set_text(current_.has_key("id") ? current_["id"].as_string() : "normal");
	id_entry->set_dropdown_height(height() - current_height - border_offset);
	id_entry->set_on_change_handler(boost::bind(&animation_creator_dialog::on_id_change, this, id_entry, _1));
	id_entry->set_on_select_handler(boost::bind(&animation_creator_dialog::on_id_set, this, id_entry, _1, _2));
	g->add_col(widget_ptr(new label("Identifier: ", graphics::color_white(), 14)))
		.add_col(widget_ptr(id_entry))
		.finish_row();

	g->add_col(button_ptr(new button(new label("Choose Image File", 14), boost::bind(&animation_creator_dialog::set_image_file, this))))
		.add_col(widget_ptr(new label(rel_path_, graphics::color_green(), 14)))
		.finish_row();

	std::pair<variant, variant> p;
	foreach(p, current_.as_map()) {
		if(p.second.is_int() && show_attribute(p.first)) {
			text_editor_widget_ptr entry(new text_editor_widget(100, 28));
			std::stringstream ss;
			ss << p.second.as_int();
			entry->set_text(ss.str());
			slider_ptr slide(new slider(200, boost::bind((&animation_creator_dialog::change_slide), this, p.first.as_string(), entry, _1), 0.5));
			slide->set_drag_end(boost::bind(&animation_creator_dialog::end_slide, this, p.first.as_string(), slide, entry, _1));
			entry->set_on_change_handler(boost::bind(&animation_creator_dialog::change_text, this, p.first.as_string(), entry, slide));
			entry->set_on_enter_handler(boost::bind(&animation_creator_dialog::execute_change_text, this, p.first.as_string(), entry, slide));
			entry->set_on_tab_handler(boost::bind(&animation_creator_dialog::execute_change_text, this, p.first.as_string(), entry, slide));
			g->add_col(widget_ptr(new label(p.first.as_string(), graphics::color_white(), 12)))
				.add_col(entry)
				.add_col(slide);
		}
	}
	add_widget(g, border_offset, current_height);
	current_height += g->height() + hpad;

	// Add/Delete animation buttons
	g.reset(new grid(4));
	g->set_hpad(50);
	g->add_col(button_ptr(new button(new label("New", 14), boost::bind(&animation_creator_dialog::anim_new, this))))
		.add_col(button_ptr(new button(new label("Save", 14), boost::bind(&animation_creator_dialog::anim_save, this, (dialog*)NULL))))
		.add_col(button_ptr(new button(new label("Delete", 14), boost::bind(&animation_creator_dialog::anim_del, this))))
		.add_col(button_ptr(new button(new label("Finish", 14), boost::bind(&animation_creator_dialog::finish, this))));
	add_widget(g, border_offset, height() - border_offset - g->height());
	current_height = height() - border_offset - g->height();

	checkbox_ptr cb = new checkbox("Simplified Options", simple_options_, boost::bind(&animation_creator_dialog::set_option, this), BUTTON_SIZE_DOUBLE_RESOLUTION);
	add_widget(cb, border_offset, current_height - cb->height() - 10);
}

void animation_creator_dialog::process()
{
	int border_offset = 35;
	try {
		if(animation_preview_widget::is_animation(current_)) {
			if(!animation_preview_) {
				animation_preview_.reset(new animation_preview_widget(current_));
				animation_preview_->set_rect_handler(boost::bind(&animation_creator_dialog::set_animation_rect, this, _1));
				animation_preview_->set_solid_handler(boost::bind(&animation_creator_dialog::move_solid_rect, this, _1, _2));
				animation_preview_->set_pad_handler(boost::bind(&animation_creator_dialog::set_integer_attr, this, "pad", _1));
				animation_preview_->set_num_frames_handler(boost::bind(&animation_creator_dialog::set_integer_attr, this, "frames", _1));
				animation_preview_->set_frames_per_row_handler(boost::bind(&animation_creator_dialog::set_integer_attr, this, "frames_per_row", _1));
				animation_preview_->set_loc(width() - int(width()*0.42) - border_offset, border_offset);
				animation_preview_->set_dim(int(width()*0.42), height()-border_offset*2);
				animation_preview_->init();
			} else {
				animation_preview_->set_object(current_);
			}
		}
	} catch (type_error&) {
		if(animation_preview_) {
			animation_preview_.reset();
		}
	} catch(frame::error&) {
		// skip
	} catch(validation_failure_exception&) {
		if(animation_preview_) {
			animation_preview_.reset();
		}
	} catch(graphics::load_image_error&) {
		if(animation_preview_) {
			animation_preview_.reset();
		}
	}

	if(animation_preview_) {
		animation_preview_->process();
	}
}

void animation_creator_dialog::handle_draw() const
{
	dialog::handle_draw();

	if(animation_preview_) {
		animation_preview_->draw();
	}

}

bool animation_creator_dialog::handle_event(const SDL_Event& event, bool claimed)
{
	if(animation_preview_) {
		claimed = animation_preview_->process_event(event, claimed) || claimed;
		if(claimed) {
			return claimed;
		}
	}
	return dialog::handle_event(event, claimed);
}

void animation_creator_dialog::set_animation_rect(rect r)
{
	if(current_.is_null() == false) {
		current_.add_attr(variant("rect"), r.write());
		changed_ = true;
	}
	init();
}

void animation_creator_dialog::move_solid_rect(int dx, int dy)
{
	if(current_.is_null() == false) {
		variant solid_area = current_["solid_area"];
		if(!solid_area.is_list() || solid_area.num_elements() != 4) {
			return;
		}

		foreach(const variant& num, solid_area.as_list()) {
			if(!num.is_int()) {
				return;
			}
		}

		rect area(solid_area);
		area = rect(area.x() + dx, area.y() + dy, area.w(), area.h());
		current_.add_attr(variant("solid_area"), area.write());
		changed_ = true;
	}
}

void animation_creator_dialog::set_integer_attr(const char* attr, int value)
{
	changed_ = true;

	//std::cerr << "set_integer_attr: " << attr << ": " << value << std::endl;
	slider_offset_[attr] = value;
	if(current_.is_null() == false) {
		current_.add_attr(variant(attr), variant(value));
	}
	init();
}

void animation_creator_dialog::on_id_change(dropdown_widget_ptr editor, const std::string& s)
{
	if(current_.is_null() == false) {
		current_.add_attr(variant("id"), variant(s));
		changed_ = true;
	}
}

void animation_creator_dialog::on_id_set(dropdown_widget_ptr editor, int selection, const std::string& s)
{
	if(current_.is_null() == false) {
		current_.add_attr(variant("id"), variant(s));
		changed_ = true;
	}
	init();
}

void animation_creator_dialog::set_image_file()
{
	gui::filter_list f;
	f.push_back(gui::filter_pair("Image Files", ".*?\\.(png|jpg|gif|bmp|tif|tiff|tga|webp|xpm|xv|pcx)"));
	f.push_back(gui::filter_pair("All Files", ".*"));
	gui::file_chooser_dialog open_dlg(
		int(preferences::virtual_screen_width()*0.1), 
		int(preferences::virtual_screen_height()*0.1), 
		int(preferences::virtual_screen_width()*0.8), 
		int(preferences::virtual_screen_height()*0.8),
		f);
	open_dlg.set_background_frame("empty_window");
	open_dlg.set_draw_background_fn(do_draw_scene);
	open_dlg.show_modal();

	if(open_dlg.cancelled() == false) {
		image_file_ = open_dlg.get_file_name();
		int offs = image_file_.rfind("/");
		image_file_name_ = image_file_.substr(offs+1);
		if(current_.is_null() == false) {
			current_.add_attr(variant("image"), variant(image_file_));
			changed_ = true;
		}
		rel_path_ = sys::compute_relative_path(module::get_module_path("") + "images", copy_path_ + "/" + image_file_name_);
	}
	init();
}

void animation_creator_dialog::change_text(const std::string& s, text_editor_widget_ptr editor, slider_ptr slide)
{
	if(!dragging_slider_) {
		int i;
		std::istringstream(editor->text()) >> i;
		slide->set_position(0.5);
		if(current_.is_null() == false) {
			current_.add_attr(variant(s), variant(i));
		}
		changed_ = true;
	}
}

void animation_creator_dialog::execute_change_text(const std::string& s, text_editor_widget_ptr editor, slider_ptr slider)
{
	if(!dragging_slider_) {
		int i;
		std::istringstream(editor->text()) >> i;
		slider->set_position(0.5);
		set_integer_attr(s.c_str(), i);
	}
}

void animation_creator_dialog::change_slide(const std::string& s, text_editor_widget_ptr editor, double d)
{
	dragging_slider_ = true;
	std::ostringstream ss;
	int i = slider_transform(d) + slider_offset_[s];
	ss << i;
	editor->set_text(ss.str());

	if(current_.is_null() == false) {
		current_.add_attr(variant(s), variant(i));
	}
	changed_ = true;
	//int soffs = slider_offset_[s];
	//set_integer_attr(s.c_str(), i);
	//slider_offset_[s] = soffs;
}

void animation_creator_dialog::end_slide(const std::string& s, slider_ptr slide, text_editor_widget_ptr editor, double d)
{
	int i = slider_transform(d) + slider_offset_[s];
	set_integer_attr(s.c_str(), i);
	slide->set_position(0.5);
	dragging_slider_ = false;
	init();
}

void animation_creator_dialog::anim_del()
{
	check_anim_changed();

	if(selected_frame_ >= 0 && size_t(selected_frame_) < anims_.size()) {
		anims_.erase(anims_.begin() + selected_frame_);
		selected_frame_ = -1;
	}
	reset_current_object();
	init();
}

void animation_creator_dialog::anim_new()
{
	check_anim_changed();
	reset_current_object();
	init();
}

void animation_creator_dialog::reset_current_object()
{
	get_default_properties().clear();
	current_ = variant(&get_default_properties());

	current_.add_attr(variant("image"), variant(image_file_));
	//image_file_name_.clear();
	//image_file_.clear();
	//rel_path_.clear();
	copy_path_ = module::get_module_path("") + "images";

	selected_frame_ = -1;

	// reset the slider offsets.
	for(std::map<std::string, int>::iterator it = slider_offset_.begin(); it != slider_offset_.end(); it++) {
		it->second = 0;
	}

	if(animation_preview_) {
		animation_preview_.reset();
	}
}


void animation_creator_dialog::anim_save(dialog* d)
{
	// Save the current animation parameters, overwriting current animation values.
	if(current_.is_null() == false) {
		if(selected_frame_ == -1) {
			// copy file
			sys::copy_file(image_file_, copy_path_ + "/" + image_file_name_);
			// important to fix up image path
			current_.add_attr(variant("image"), variant(rel_path_));

			// erase any properties still as defaults.
			std::map<variant, variant>::const_iterator vit = current_.as_map().begin();
			std::map<variant, variant>::const_iterator end = current_.as_map().end();
			while(vit != end) {
				std::map<variant, variant>::const_iterator dit = get_default_properties().find(vit->first);
				if(dit != get_default_properties().end()) {
					if(vit->second == dit->second) {
						current_.remove_attr((vit++)->first);
					} else {
						vit++;
					}
				} else {
					vit++;
				}
			}

			// add the animation to list
			anims_.push_back(current_);
		} else {
			anims_[selected_frame_] = current_;
		}
	}
	changed_ = false;
	reset_current_object();

	if(d) {
		d->close();
	} else {
		init();
	}
}

void animation_creator_dialog::check_anim_changed()
{
	if(changed_) {
		// Create message box with Save/Cancel options.
		dialog d((width()-400)/2, (height()-300)/2, 400, 300);
		d.set_background_frame("empty_window");
		d.set_padding(20);
		
		label_ptr title = new label("Animation has changed.", graphics::color_white(), 24);
		d.add_widget(title, (d.width()-title->width())/2, 50);
		grid_ptr g = new grid(2);
		g->set_header_row(40);
		g->add_col(widget_ptr(new button("Save", boost::bind(&animation_creator_dialog::anim_save, this, &d))))
			.add_col(widget_ptr(new button("Discard", boost::bind(&dialog::cancel, &d))));
		d.add_widget(g, (d.width()-g->width())/2, 30+70+title->height());
		d.show_modal();
		changed_ = false;
		init();
	}
}

void animation_creator_dialog::select_animation(int index)
{
	// index 0 is our label row.
	if(index < 1 || size_t(index) > anims_.size()) {
		return;
	}
	check_anim_changed();

	selected_frame_ = index - 1;
	current_ = anims_[selected_frame_];
	variant_map_merge(current_, variant(&get_default_properties()));
	init();
}

void animation_creator_dialog::set_destination()
{
	file_chooser_dialog dir_dlg(
		int(preferences::virtual_screen_width()*0.2), 
		int(preferences::virtual_screen_height()*0.2), 
		int(preferences::virtual_screen_width()*0.6), 
		int(preferences::virtual_screen_height()*0.6),
		gui::filter_list(), 
		true, module::get_module_path("") + "images");
	dir_dlg.set_background_frame("empty_window");
	dir_dlg.set_draw_background_fn(do_draw_scene);
	dir_dlg.use_relative_paths(true);
	dir_dlg.show_modal();

	if(dir_dlg.cancelled() == false) {
		copy_path_ = dir_dlg.get_path();
		rel_path_ = sys::compute_relative_path(module::get_module_path("") + "images", copy_path_ + "/" + image_file_name_);
	}
	init();
}

void animation_creator_dialog::finish()
{
	check_anim_changed();
	close();
}

void animation_creator_dialog::set_option()
{
	simple_options_ = !simple_options_;
	init();
}

bool animation_creator_dialog::show_attribute(variant v)
{
	if(!simple_options_) {
		return true;
	}
	std::string s = v.as_string();
	if(s == "frames" || s == "frames_per_row" || s == "duration" || s == "pad") {
		return true;
	}
	return false;
}

}

#endif // NO_EDITOR
