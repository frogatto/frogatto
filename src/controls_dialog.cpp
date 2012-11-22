#include <boost/bind.hpp>

#include "button.hpp"
#include "controls.hpp"
#include "controls_dialog.hpp"
#include "dialog.hpp"
#include "draw_scene.hpp"
#include "graphical_font_label.hpp"
#include "i18n.hpp"
#include "level.hpp"
#include "key_button.hpp"
#include "preferences.hpp"

namespace {
gui::key_button_ptr key_buttons[controls::NUM_CONTROLS];

void end_dialog(gui::dialog* d)
{
	using namespace controls;
	for(int n = 0; n < NUM_CONTROLS; ++n) {
		const CONTROL_ITEM item = static_cast<CONTROL_ITEM>(n);
		set_sdlkey(item, key_buttons[item]->get_key());
	}
	d->close();
}

void do_draw_scene() {
	draw_scene(level::current(), last_draw_position());
}

}

void show_controls_dialog()
{
	using namespace gui;
	using namespace controls;
	int height = preferences::virtual_screen_height() - 20;
	if (preferences::virtual_screen_height() > 480)
		height -= 100;
	dialog d(200, (preferences::virtual_screen_height() > 480) ? 60 : 10, preferences::virtual_screen_width()-400, height);
	d.set_background_frame("empty_window");
	d.set_draw_background_fn(do_draw_scene);


	for(int n = 0; n < NUM_CONTROLS; ++n) {
		const CONTROL_ITEM item = static_cast<CONTROL_ITEM>(n);
		key_buttons[item] = key_button_ptr(new key_button(get_sdlkey(item), BUTTON_SIZE_DOUBLE_RESOLUTION));
		key_buttons[item]->set_dim(70, 60);
	}

	widget_ptr t1(new graphical_font_label(_("Directions"), "door_label", 2));
	widget_ptr b1(key_buttons[CONTROL_UP]);
	widget_ptr b2(key_buttons[CONTROL_DOWN]);
	widget_ptr b3(key_buttons[CONTROL_LEFT]);
	widget_ptr b4(key_buttons[CONTROL_RIGHT]);
	widget_ptr t2(new graphical_font_label(_("Jump"), "door_label", 2));
	widget_ptr b5(key_buttons[CONTROL_JUMP]);
	widget_ptr t3(new graphical_font_label(_("Tongue"), "door_label", 2));
	widget_ptr b6(key_buttons[CONTROL_TONGUE]);
	widget_ptr t4(new graphical_font_label(_("Attack"), "door_label", 2));
	widget_ptr b7(key_buttons[CONTROL_ATTACK]);
	widget_ptr b8(new button(widget_ptr(new graphical_font_label(_("Back"), "door_label", 2)), boost::bind(end_dialog, &d), BUTTON_STYLE_DEFAULT, BUTTON_SIZE_DOUBLE_RESOLUTION));
	b8->set_dim(230, 60);

	int start_y = (d.height() - 4*b1->height() - 2*t1->height() - 7*d.padding())/2;
	d.add_widget(t1, d.width()/2 - b1->width()*1.5 - d.padding(), start_y);
	d.add_widget(b1, d.width()/2 - b1->width()/2, start_y + t1->height() + d.padding());
	d.add_widget(b3, d.width()/2 - b1->width()*1.5 - d.padding(), start_y + t1->height() + b1->height() + 2*d.padding(), dialog::MOVE_RIGHT);
	d.add_widget(b2, dialog::MOVE_RIGHT);
	d.add_widget(b4);

	start_y += t1->height() + 5*d.padding() + 2*b1->height();
	d.add_widget(t2, d.width()/2 - b1->width()*1.5 - d.padding(), start_y);
	d.add_widget(b5);
	d.add_widget(t3, d.width()/2 - b1->width()/2, start_y);
	d.add_widget(b6);
	d.add_widget(t4, d.width()/2 + b1->width()/2 + d.padding(), start_y);
	d.add_widget(b7);
	d.add_widget(b8, d.width()/2 - b8->width()/2, start_y + t2->height() + b5->height() + 3*d.padding());

	d.show_modal();
}
