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
		void draw_ticks(GLfloat x_offset, int segments, const SDL_Color& color) const;

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
		SDL_Color drained_bar_color_;
		SDL_Color tick_mark_color_;
		SDL_Color drained_tick_mark_color_;

		GLfloat rotate_;
		GLfloat scale_;

		// Rate (in frames/segment) that the bar goes removes
		// segments when missing_segments_ is increased or decreased.
		double drain_rate_;

		// Whether we are animating a transition or not.
		bool animating_;

		// Segments is a reflection of the number of ticks in the bar.
		// One segment = no tick marks
		// two segments = 1 tick 
		// and so on
		int segments_;

		// Segments drawn with alternative color
		int drained_segments_;

		// Length of segments, in pixels basically.
		int segment_length_;

		// Width of tick mark in pixels.
		int tick_width_;

		// Calculated total length of the bar
		int total_bar_length_;
		// Calculated active length of the bar (i.e. segments-drained_segments)
		int active_bar_length_;
		// Calculated length of the drained bar
		int drained_bar_length_;
		// Calculated left_cap_width
		int left_cap_width_;
		// Calculated right_cap_width
		int right_cap_width_;
	};
}

#endif
