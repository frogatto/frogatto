#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <sstream>

#include "button.hpp"
#include "foreach.hpp"
#include "grid_widget.hpp"
#include "image_widget.hpp"
#include "label.hpp"
#include "property_editor_dialog.hpp"
#include "raster.hpp"

namespace editor_dialogs
{

property_editor_dialog::property_editor_dialog(editor& e)
  : gui::dialog(640, 40, 160, 560), editor_(e)
{
	init();
}

void property_editor_dialog::init()
{
	clear();
	if(!entity_) {
		return;
	}

	using namespace gui;

	set_padding(20);

	const frame& frame = entity_->current_frame();
	image_widget* preview = new image_widget(frame.img());
	preview->set_dim(frame.width(), frame.height());
	preview->set_area(frame.area());
	add_widget(widget_ptr(preview), 10, 10);

	game_logic::formula_callable* vars = entity_->vars();
	if(vars) {
		std::vector<game_logic::formula_input> inputs;
		vars->get_inputs(&inputs);
		foreach(const game_logic::formula_input& in, inputs) {
			std::ostringstream s;
			s << in.name << ": " << vars->query_value(in.name).to_debug_string();
			label_ptr lb = label::create(s.str(), graphics::color_white());
			add_widget(widget_ptr(lb));

			grid_ptr buttons_grid(new grid(4));
			buttons_grid->add_col(widget_ptr(new button(widget_ptr(new label("-10", graphics::color_white())), boost::bind(&property_editor_dialog::change_property, this, in.name, -10))));
			buttons_grid->add_col(widget_ptr(new button(widget_ptr(new label("-1", graphics::color_white())), boost::bind(&property_editor_dialog::change_property, this, in.name, -1))));
			buttons_grid->add_col(widget_ptr(new button(widget_ptr(new label("+1", graphics::color_white())), boost::bind(&property_editor_dialog::change_property, this, in.name, +1))));
			buttons_grid->add_col(widget_ptr(new button(widget_ptr(new label("+10", graphics::color_white())), boost::bind(&property_editor_dialog::change_property, this, in.name, +10))));
			add_widget(widget_ptr(buttons_grid));
		}
	}
}

void property_editor_dialog::set_entity(entity_ptr e)
{
	entity_ = e;
	init();
}

void property_editor_dialog::change_property(const std::string& id, int change)
{
	game_logic::formula_callable* vars = entity_->vars();
	if(vars) {
		vars->mutate_value(id, vars->query_value(id) + variant(change));
		init();
	}
}

}
