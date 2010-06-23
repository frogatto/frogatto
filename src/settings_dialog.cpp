#include "settings_dialog.hpp"

#include "gui_section.hpp"
#include "preferences.hpp"
#include "raster.hpp"

namespace
{
	const int sw = graphics::screen_width();
	const int sh = graphics::screen_height();
	const int padding = 20;
}

void settings_dialog::draw () const
{
	const const_gui_section_ptr menu_button = menu_button_state_ ? menu_button_down_ : menu_button_normal_;
	menu_button->blit(sw - menu_button->width() - padding, padding);
}

bool settings_dialog::handle_event (const SDL_Event& event)
{
	if (event.type == SDL_MOUSEMOTION || event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP)
	{
		const int menu_button_x = sw - menu_button_normal_->width() - padding;
		const int menu_button_y = padding;
		int x = event.type == SDL_MOUSEMOTION ? event.motion.x : event.button.x;
		int y = event.type == SDL_MOUSEMOTION ? event.motion.y : event.button.y;
		bool hittest = (x > menu_button_x && y > menu_button_y
			&& x < menu_button_x+menu_button_normal_->width() && y < menu_button_y+menu_button_normal_->height());
		if (hittest && (event.type == SDL_MOUSEBUTTONDOWN || (event.type == SDL_MOUSEMOTION && event.motion.state)))
		{
			menu_button_state_ = true;
		} else {
			menu_button_state_ = false;
		}
		if (hittest && event.type == SDL_MOUSEBUTTONUP)
		{
			show_window_ = true;
		}
	}
	return show_window_;
}

void settings_dialog::reset ()
{
	show_window_ = false;
	menu_button_state_ = false;
}

settings_dialog::settings_dialog () : show_window_(false), menu_button_state_(false),
	menu_button_normal_(gui_section::get("menu_button_normal")),
	menu_button_down_(gui_section::get("menu_button_down"))
{
}

settings_dialog::~settings_dialog () {}
