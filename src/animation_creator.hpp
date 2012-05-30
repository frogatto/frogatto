#pragma once
#ifndef ANIMATION_CREATOR_HPP_INCLUDED
#define ANIMATION_CREATOR_HPP_INCLUDED
#ifndef NO_EDITOR

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <vector>
#include <map>

#include "button.hpp"
#include "dialog.hpp"
#include "dropdown_widget.hpp"
#include "frame.hpp"
#include "graphics.hpp"
#include "grid_widget.hpp"
#include "label.hpp"
#include "raster.hpp"
#include "slider.hpp"
#include "text_editor_widget.hpp"

namespace gui {

class animation_creator_dialog : public virtual dialog
{
public:
	animation_creator_dialog(int x, int y, int w, int h, const variant& anims=variant());
	virtual ~animation_creator_dialog() 
	{}
	variant get_animations();
protected:
	void init();
	
	void set_destination(label_ptr copy_dest);
	void select_animation(int index);
	void anim_add();
	void anim_del();
	void anim_new();

	virtual void handle_draw() const;
private:
	variant anims_;
	mutable int cycle_;
	int selected_frame_;
	rect preview_area_;
	std::vector<boost::shared_ptr<frame> > frames_;

	// Currently display animation variables (a subset)
	std::string id_;
	std::string image_file_;
	rect image_rect_;
	bool reverse_;
	bool rotate_on_slope_;
	rect collide_rect_;
	rect hit_rect_;
	rect platform_rect_;

	void set_properties();
	void set_default_properties();
	std::map<std::string, int> int_properties_;

	void on_id_change();
	void set_image_file();
	void change_text(const std::string& s, text_editor_widget_ptr editor, slider_ptr slider);
	void change_slide(const std::string& s, text_editor_widget_ptr editor, double d);
	void end_slide(const std::string& s, slider_ptr slide, text_editor_widget_ptr editor, double d);
	
	std::map<std::string, int> slider_offset_;
	bool dragging_slider_;
};

}

#endif // NO_EDITOR
#endif // ANIMATION_CREATOR_HPP_INCLUDED
