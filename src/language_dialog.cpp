
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

class grid {
	gui::dialog& dialog_;
	int cell_width_;
	int cell_height_;
	int h_padding_;
	int v_padding_;
	int start_x_;
	int start_y_;
	int column_count_;
	int widget_count_;

	public:
	grid(gui::dialog& dialog, int cell_width, int cell_height, int h_padding, int v_padding, int start_x, int start_y, int column_count) :
   dialog_(dialog), cell_width_(cell_width), cell_height_(cell_height), h_padding_(h_padding), v_padding_(v_padding), start_x_(start_x), start_y_(start_y), column_count_(column_count), widget_count_(0) {
	}

	void add_widget(gui::widget_ptr widget) {
		dialog_.add_widget(widget,
			start_x_ + h_padding_ + (widget_count_ % column_count_) * (cell_width_ + h_padding_),
			start_y_ + v_padding_ + (widget_count_ / column_count_) * (cell_height_ + v_padding_));
		widget_count_++;
	}

	int total_width() {
		return start_x_ + column_count_ * (cell_width_ + h_padding_);
	}

	int total_height() {
		return start_y_ + (widget_count_ + column_count_ - 1) / column_count_ * (cell_height_ + v_padding_);
	}
};
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

	const int button_width = 300;
	const int button_height = 50;
	const int padding = 20;

	d.add_widget(widget_ptr(new graphical_font_label(_("Language change will take effect in next level."), "door_label", 2)), padding, padding);

	grid g(d, button_width, button_height, padding, padding, 0, 40, 2);

	typedef std::map<variant, variant> variant_map;
	variant_map languages = json::parse_from_file("data/languages.cfg").as_map();
	int index = 0;
	foreach(variant_map::value_type pair, languages) {
		widget_ptr b(new button(
			widget_ptr(new graphical_font_label(pair.second.as_string(), "door_label", 2)),
			boost::bind(set_locale, pair.first.as_string()),
			BUTTON_STYLE_NORMAL, BUTTON_SIZE_DOUBLE_RESOLUTION));
		b->set_dim(button_width, button_height);
		g.add_widget(b);
	}

	widget_ptr system_button(new button(
		widget_ptr(new graphical_font_label(_("Use system language"), "door_label", 2)),
	   	boost::bind(set_locale, "system"),
		BUTTON_STYLE_NORMAL, BUTTON_SIZE_DOUBLE_RESOLUTION));
	system_button->set_dim(button_width, button_height);
	g.add_widget(system_button);

	widget_ptr back_button(new button(widget_ptr(new graphical_font_label(_("Back"), "door_label", 2)), boost::bind(end_dialog, &d), BUTTON_STYLE_DEFAULT, BUTTON_SIZE_DOUBLE_RESOLUTION));
	back_button->set_dim(button_width, button_height);
	g.add_widget(back_button);

	d.set_dim(g.total_width() + padding, g.total_height() + padding);

	d.show_modal();
}
