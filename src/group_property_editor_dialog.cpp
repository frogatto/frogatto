#ifndef NO_EDITOR
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <sstream>

#include "button.hpp"
#include "editor_dialogs.hpp"
#include "foreach.hpp"
#include "grid_widget.hpp"
#include "group_property_editor_dialog.hpp"
#include "image_widget.hpp"
#include "label.hpp"
#include "load_level.hpp"
#include "raster.hpp"

namespace editor_dialogs
{

group_property_editor_dialog::group_property_editor_dialog(editor& e)
  : gui::dialog(graphics::screen_width() - 160, 160, 160, 440), editor_(e)
{
	group_ = e.get_level().editor_selection();
	init();
}

void group_property_editor_dialog::init()
{
	clear();

	using namespace gui;
	set_padding(20);

	if(group_.empty() == false) {
		std::cerr << "ADD BUTTON\n";
		add_widget(widget_ptr(new button(widget_ptr(new label("Group Objects", graphics::color_white())), boost::bind(&group_property_editor_dialog::group_objects, this))), 10, 10);
	}
}

void group_property_editor_dialog::set_group(const std::vector<entity_ptr>& group)
{
	group_ = group;
	init();
}

void group_property_editor_dialog::group_objects()
{
	editor_.group_selection();
}

}
#endif // !NO_EDITOR
