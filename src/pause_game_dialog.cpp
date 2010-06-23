#include <boost/bind.hpp>

#include "button.hpp"
#include "dialog.hpp"
#include "graphical_font_label.hpp"
#include "pause_game_dialog.hpp"
#include "preferences.hpp"

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

	using namespace gui;
	dialog d(0, 0, preferences::actual_screen_width(), preferences::actual_screen_height());
	widget_ptr b1(new button(widget_ptr(new graphical_font_label("Resume", "default", 2)), boost::bind(end_dialog, &d, &result, PAUSE_GAME_CONTINUE)));
	widget_ptr b2(new button(widget_ptr(new graphical_font_label("Exit to Titlescreen", "default", 2)), boost::bind(end_dialog, &d, &result, PAUSE_GAME_GO_TO_TITLESCREEN)));
	widget_ptr b3(new button(widget_ptr(new graphical_font_label("Exit Game", "default", 2)), boost::bind(end_dialog, &d, &result, PAUSE_GAME_QUIT)));

	b1->set_dim(400, 100);
	b2->set_dim(400, 100);
	b3->set_dim(400, 100);

	d.add_widget(b1, preferences::actual_screen_width()/2 - b1->width()/2, preferences::actual_screen_height()/2 - b1->height()*2);
	d.add_widget(b2);
	d.add_widget(b3);


	d.show_modal();
	if(d.cancelled() && result == PAUSE_GAME_QUIT) {
		result = PAUSE_GAME_CONTINUE;
	}

	return result;
}
