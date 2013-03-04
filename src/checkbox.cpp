#include <boost/bind.hpp>

#include "checkbox.hpp"
#include "graphical_font_label.hpp"
#include "grid_widget.hpp"
#include "image_widget.hpp"
#include "raster.hpp"
#include "widget_factory.hpp"

namespace gui {
namespace {

widget_ptr create_checkbox_widget(widget_ptr label, 
	bool checked, 
	BUTTON_RESOLUTION resolution,
	int hpadding=12) {
	grid_ptr g(new grid(2));
	g->set_hpad(hpadding);
	g->add_col(widget_ptr(new gui_section_widget(checked ? "checkbox_ticked" : "checkbox_unticked", -1, -1, resolution == BUTTON_SIZE_NORMAL_RESOLUTION ? 1 : 2)));

	g->add_col(label);

	return g;
}

widget_ptr create_checkbox_widget(const std::string& text, bool checked, BUTTON_RESOLUTION resolution) {
	return create_checkbox_widget(widget_ptr(new graphical_font_label(text, "door_label", 2)), checked, resolution);
}
}

checkbox::checkbox(const std::string& label, bool checked, boost::function<void(bool)> onclick, BUTTON_RESOLUTION button_resolution)
  : button(create_checkbox_widget(label, checked, button_resolution), boost::bind(&checkbox::on_click, this), BUTTON_STYLE_NORMAL,button_resolution), label_(label), onclick_(onclick), checked_(checked), hpadding_(12)
{
	set_environment();
}

checkbox::checkbox(widget_ptr label, bool checked, boost::function<void(bool)> onclick, BUTTON_RESOLUTION button_resolution)
  : button(create_checkbox_widget(label, checked, button_resolution), boost::bind(&checkbox::on_click, this), BUTTON_STYLE_NORMAL,button_resolution), label_widget_(label), onclick_(onclick), checked_(checked), hpadding_(12)
{
	set_environment();
}

checkbox::checkbox(const variant& v, game_logic::formula_callable* e) 
	: checked_(false), button(v,e)
{
	hpadding_ = v["hpad"].as_int(12);
	if(v.has_key("padding")) {
		ASSERT_LOG(v["padding"].num_elements() == 2, "Incorrect number of padding elements specifed." << v["padding"].num_elements());
		hpadding_ = v["padding"][0].as_int();
	}
	checked_ = v["checked"].as_bool(false);
	variant label_var = v["text"];
	label_ = (label_var.is_map() || label_var.is_callable()) ? "" : label_var.as_string_default("Button");
	label_widget_ = (label_var.is_map() || label_var.is_callable())
		? widget_factory::create(label_var, e) 
		: widget_ptr(new graphical_font_label(label_, "door_label", 2));
	ASSERT_LOG(get_environment() != 0, "You must specify a callable environment");
	click_handler_ = get_environment()->create_formula(v["on_click"]);
	onclick_ = boost::bind(&checkbox::click, this, _1);
	set_click_handler(boost::bind(&checkbox::on_click, this));

	set_label(create_checkbox_widget(label_widget_, 
		checked_, 
		button_resolution(),
		hpadding_));
}

void checkbox::on_click()
{
	checked_ = !checked_;
	const int w = width();
	const int h = height();
	if(label_widget_) {
		set_label(create_checkbox_widget(label_widget_, checked_, button_resolution(), hpadding_));
	} else {
		set_label(create_checkbox_widget(label_, checked_, button_resolution()));
	}
	set_dim(w, h);
	onclick_(checked_);
}

void checkbox::click(bool checked)
{
	using namespace game_logic;
	if(get_environment()) {
		map_formula_callable_ptr callable = map_formula_callable_ptr(new map_formula_callable(get_environment()));
		callable->add("checked", variant::from_bool(checked));
		variant value = click_handler_->execute(*callable);
		get_environment()->execute_command(value);
	} else {
		std::cerr << "checkbox::click() called without environment!" << std::endl;
	}
}

variant checkbox::get_value(const std::string& key) const
{
	if(key == "checked") {
		return variant::from_bool(checked_);
	} else if(key == "label") {
		return variant(label_widget_.get());
	}
	return button::get_value(key);
}

}
