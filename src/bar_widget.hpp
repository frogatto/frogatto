/*
	Copyright (C) 2003-2013 by David White <davewx7@gmail.com>
	
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
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
		virtual void handle_draw() const;
		virtual void handle_process();
		virtual bool handle_event(const SDL_Event& event, bool claimed);
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

		GLfloat animation_end_point_unscaled_;
		GLfloat animation_current_position_;

		// Rate (in frames/segment) that the bar goes removes
		// segments when drained_segments_ is increased or decreased.
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
		// Calculated distance between ticks
		double tick_distance_;

		// Saved bar_height and width;
		int bar_height_;
		int bar_max_width_;

		int drained_segments_after_anim_;
	};
}

#endif
