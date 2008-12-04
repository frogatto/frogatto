#include <boost/bind.hpp>

#include "button.hpp"
#include "editor.hpp"
#include "foreach.hpp"
#include "grid_widget.hpp"
#include "image_widget.hpp"
#include "label.hpp"
#include "prop_editor_dialog.hpp"
#include "raster.hpp"

namespace editor_dialogs
{

prop_editor_dialog::prop_editor_dialog(editor& e)
  : gui::dialog(640, 40, 160, 560), editor_(e)
{
	if(editor_.get_props().empty() == false) {
		category_ = editor_.get_props().front()->category();
	}
	init();
}

void prop_editor_dialog::init()
{
	clear();
	using namespace gui;
	set_padding(20);

	const_prop_ptr prop = editor_.get_props()[editor_.get_item()];
	image_widget* preview = new image_widget(prop->get_frame().img());
	preview->set_dim(150, 150);
	preview->set_area(prop->get_frame().area());
	add_widget(widget_ptr(preview), 10, 10);

	button* category_button = new button(widget_ptr(new label(category_, graphics::color_white())), boost::bind(&prop_editor_dialog::show_category_menu, this));
	add_widget(widget_ptr(category_button));

	grid_ptr grid(new gui::grid(2));
	int index = 0;
	foreach(const_prop_ptr prop, editor_.get_props()) {
		if(prop->category() == category_) {
			image_widget* preview = new image_widget(prop->get_frame().img());
			preview->set_dim(64, 64);
			preview->set_area(prop->get_frame().area());
			button_ptr prop_button(new button(widget_ptr(preview), boost::bind(&prop_editor_dialog::set_prop, this, index)));
			prop_button->set_dim(68, 68);
			grid->add_col(prop_button);
		}

		++index;
	}

	grid->finish_row();
	add_widget(grid);
}

void prop_editor_dialog::show_category_menu()
{
}

void prop_editor_dialog::set_prop(int index)
{
	editor_.set_item(index);
	init();
}

}
