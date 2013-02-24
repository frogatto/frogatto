#pragma once
#ifndef BAR_WIDGET_HPP_INCLUDED
#define BAR_WIDGET_HPP_INCLUDED

#include "widget.hpp"

namespace gui
{
	class bar_widget : public widget
	{
	public:
		bar_widget(const variant& v, game_logic::formula_callable* e);
		virtual ~bar_widget();
		void init();

		void set_rotation(GLfloat rotate);
	private:
		virtual variant get_value(const std::string& key) const;
		virtual void set_value(const std::string& key, const variant& value);
	protected:
		void handle_draw() const;
		bool handle_event(const SDL_Event& event, bool claimed);

		struct bar_section
		{
			graphics::texture texture;
			rect area;
		};

		void init_bar_section(const variant&v, bar_section* b);

		bar_section bar_;
		bar_section left_cap_;
		bar_section right_cap_;

		SDL_Color bar_color_;
		SDL_Color tick_mark_color_;

		GLfloat rotate_;
		GLfloat scale_;

		// Segments is a reflection of the number of ticks in the bar.
		// One segment = no tick marks
		// two segments = 1 tick 
		// and so on
		int segments_;

		// Length of segments, in pixels basically.
		int segment_length_;

		// Width of tick mark in pixels.
		int tick_width_;
	};
}

#endif
