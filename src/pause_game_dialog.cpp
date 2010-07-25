#include <boost/bind.hpp>

#include "button.hpp"
#include "slider.hpp"
#include "dialog.hpp"
#include "graphical_font_label.hpp"
#include "pause_game_dialog.hpp"
#include "preferences.hpp"
#include "sound.hpp"

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
	
	bool show_exit = true;
	
#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
	show_exit = false;
#endif

	bool upscale = true;
	
	using namespace gui;
	dialog d(0, 0, preferences::virtual_screen_width(), preferences::virtual_screen_height());
	widget_ptr t1(new graphical_font_label("Music Volume:", "door_label", 2));
	widget_ptr s1(new slider(200, boost::bind(sound::set_music_volume, _1), sound::get_music_volume()));
	widget_ptr t2(new graphical_font_label("Sound Volume:", "door_label", 2));
	widget_ptr s2(new slider(200, boost::bind(sound::set_sound_volume, _1), sound::get_sound_volume()));
	widget_ptr b1(new button(widget_ptr(new graphical_font_label("Resume", "door_label", 2)), boost::bind(end_dialog, &d, &result, PAUSE_GAME_CONTINUE), upscale));
	widget_ptr b2(new button(widget_ptr(new graphical_font_label("Return to Titlescreen", "door_label", 2)), boost::bind(end_dialog, &d, &result, PAUSE_GAME_GO_TO_TITLESCREEN), upscale));
	widget_ptr b3(new button(widget_ptr(new graphical_font_label("Exit Game", "door_label", 2)), boost::bind(end_dialog, &d, &result, PAUSE_GAME_QUIT), upscale));
	
	b1->set_dim(230, 60);
	b2->set_dim(230, 60);
	if (show_exit) b3->set_dim(230, 60);
	
	d.add_widget(t1, preferences::virtual_screen_width()/2 - b1->width()/2, preferences::virtual_screen_height()/2 - b1->height()*(show_exit ? 1.5 : 1));
	d.add_widget(s1);
	d.add_widget(t2);
	d.add_widget(s2);
	d.add_widget(b1);
	d.add_widget(b2);
	if (show_exit) d.add_widget(b3);


	d.show_modal();
	if(d.cancelled() && result == PAUSE_GAME_QUIT) {
		result = PAUSE_GAME_CONTINUE;
	}

	return result;
}
