
#include <boost/bind.hpp>

#include "button.hpp"
#include "language_dialog.hpp"
#include "dialog.hpp"
#include "draw_scene.hpp"
#include "graphical_font_label.hpp"
#include "i18n.hpp"
#include "level.hpp"
#include "preferences.hpp"
#include "json_parser.hpp"
#include "foreach.hpp"

namespace {
void end_dialog(gui::dialog* d)
{

	d->close();
}

void do_draw_scene() {
	draw_scene(level::current(), last_draw_position());
}

void set_locale(const std::string& value) {
	preferences::set_locale(value);
	i18n::init();
	graphical_font::init_for_locale(i18n::get_locale());
}
}

void show_language_dialog()
{
	using namespace gui;
	int height = preferences::virtual_screen_height() - 20;
	if (preferences::virtual_screen_height() > 480)
		height -= 100;
	dialog d(50, (preferences::virtual_screen_height() > 480) ? 60 : 10, preferences::virtual_screen_width()-100, height);
	d.set_background_frame("empty_window");
	d.set_draw_background_fn(do_draw_scene);

	typedef std::map<variant, variant> variant_map;
	variant_map languages = json::parse_from_file("data/languages.cfg").as_map();
	foreach(variant_map::value_type pair, languages) {
		widget_ptr b(new button(
			widget_ptr(new graphical_font_label(pair.second.as_string(), "door_label", 2)),
			boost::bind(set_locale, pair.first.as_string()),
			BUTTON_STYLE_NORMAL, BUTTON_SIZE_DOUBLE_RESOLUTION));
		d.add_widget(b);
	}

	widget_ptr system_button(new button(
		widget_ptr(new graphical_font_label(_("Use system language"), "door_label", 2)),
	   	boost::bind(set_locale, "system"),
		BUTTON_STYLE_NORMAL, BUTTON_SIZE_DOUBLE_RESOLUTION));
	d.add_widget(system_button);

	widget_ptr back_button(new button(widget_ptr(new graphical_font_label(_("Back"), "door_label", 2)), boost::bind(end_dialog, &d), BUTTON_STYLE_DEFAULT, BUTTON_SIZE_DOUBLE_RESOLUTION));
	d.add_widget(back_button);

	d.show_modal();
}
