#include <boost/bind.hpp>

#include "button.hpp"
#include "controls_dialog.hpp"
#include "slider.hpp"
#include "checkbox.hpp"
#include "dialog.hpp"
#include "graphical_font_label.hpp"
#include "i18n.hpp"
#include "pause_game_dialog.hpp"
#include "preferences.hpp"
#include "sound.hpp"
#include "of_bridge.h"

namespace {
void end_dialog(gui::dialog* d, PAUSE_GAME_RESULT* result, PAUSE_GAME_RESULT value)
{
	*result = value;
	d->close();
}

}

PAUSE_GAME_RESULT show_pause_game_dialog()
{
	PAUSE_GAME_RESULT result = PAUSE_GAME_QUIT;
	
	const int button_width = 232;
	const int button_height = 50;
	const int padding = 20;
	bool show_exit = true;
	bool show_controls = true;
	bool show_button_swap = false;
	bool show_of = false;
	
#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
	show_exit = false;
	show_controls = false;
	show_button_swap = true;
	show_of = true;
#elif TARGET_BLACKBERRY
	show_exit = false;
	show_controls = false;
#endif
	
	using namespace gui;
	widget_ptr t1(new graphical_font_label(_("Music Volume:"), "door_label", 2));
	widget_ptr s1(new slider(200, boost::bind(sound::set_music_volume, _1), sound::get_music_volume()));
	widget_ptr t2(new graphical_font_label(_("Sound Volume:"), "door_label", 2));
	widget_ptr s2(new slider(200, boost::bind(sound::set_sound_volume, _1), sound::get_sound_volume()));
	
	const int num_buttons = 2 + show_exit + show_controls + show_button_swap + show_of;
	int window_w, window_h;
	if(preferences::virtual_screen_height() >= 600) {
		window_w = button_width + padding*4;
		window_h = button_height * num_buttons + t1->height()*2 + s1->height()*2 + padding*(3+4+num_buttons);
	} else {
		window_w = button_width*2 + padding*5;
		window_h = button_height * num_buttons/2 + t1->height() + s1->height() + padding*(3+2+num_buttons/2);
	}
	dialog d((preferences::virtual_screen_width()/2 - window_w/2) & ~1, (preferences::virtual_screen_height()/2 - window_h/2) & ~1, window_w, window_h);
	d.set_padding(padding);
	
	widget_ptr b1(new button(widget_ptr(new graphical_font_label(_("Resume"), "door_label", 2)), boost::bind(end_dialog, &d, &result, PAUSE_GAME_CONTINUE), BUTTON_STYLE_NORMAL, BUTTON_SIZE_DOUBLE_RESOLUTION));
	widget_ptr b2(new button(widget_ptr(new graphical_font_label(_("Controls..."), "door_label", 2)), show_controls_dialog, BUTTON_STYLE_NORMAL, BUTTON_SIZE_DOUBLE_RESOLUTION));
	widget_ptr b3(new button(widget_ptr(new graphical_font_label(_("Return to Titlescreen"), "door_label", 2)), boost::bind(end_dialog, &d, &result, PAUSE_GAME_GO_TO_TITLESCREEN), BUTTON_STYLE_NORMAL, BUTTON_SIZE_DOUBLE_RESOLUTION));
	widget_ptr b4(new button(widget_ptr(new graphical_font_label(_("Exit Game"), "door_label", 2)), boost::bind(end_dialog, &d, &result, PAUSE_GAME_QUIT), BUTTON_STYLE_DEFAULT, BUTTON_SIZE_DOUBLE_RESOLUTION));
	widget_ptr b5(new checkbox(_("Reverse A and B"), preferences::reverse_ab(), boost::bind(preferences::set_reverse_ab, _1), BUTTON_SIZE_DOUBLE_RESOLUTION));
#ifdef ENABLE_OPENFEINT
	widget_ptr b6(new button(widget_ptr(new graphical_font_label(_("OpenFeint"), "door_label", 2)), of_dashboard, BUTTON_STYLE_NORMAL, BUTTON_SIZE_DOUBLE_RESOLUTION));
#endif
	
	b1->set_dim(button_width, button_height);
	b2->set_dim(button_width, button_height);
	b3->set_dim(button_width, button_height);
	b4->set_dim(button_width, button_height);
	b5->set_dim(button_width, button_height);
#ifdef ENABLE_OPENFEINT
	b6->set_dim(button_width, button_height);
#endif
	
	d.set_padding(padding-16);
	d.add_widget(t1, padding*2, padding*2);
	d.set_padding(padding+16);
	d.add_widget(s1);

	if(preferences::virtual_screen_height() >= 600) {
		d.set_padding(padding-16);
		d.add_widget(t2);
		d.set_padding(padding+16);
		d.add_widget(s2);
		d.set_padding(padding);
		if (show_button_swap) d.add_widget(b5);
		d.add_widget(b1);
		if (show_controls) d.add_widget(b2);
#ifdef ENABLE_OPENFEINT
		if (show_of) d.add_widget(b6);
#endif
		d.add_widget(b3);
		if (show_exit) d.add_widget(b4);
	} else {
		d.set_padding(padding);
		d.add_widget(b1);
		if (show_controls) d.add_widget(b2);
		d.set_padding(padding-16);
		d.add_widget(t2, padding*3 + button_width, padding*2);
		d.set_padding(padding+16);
		d.add_widget(s2);
		d.set_padding(padding);
		d.add_widget(b3);
		if (show_exit) d.add_widget(b4);
	}

	d.set_on_quit(boost::bind(end_dialog, &d, &result, PAUSE_GAME_QUIT));
	d.show_modal();
	if(d.cancelled() && result == PAUSE_GAME_QUIT) {
		result = PAUSE_GAME_CONTINUE;
	}

	return result;
}
