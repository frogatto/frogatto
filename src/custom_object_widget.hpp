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
#ifndef CUSTOM_OBJECT_WIDGET_HPP_INCLUDED
#define CUSTOM_OBJECT_WIDGET_HPP_INCLUDED

#include <boost/intrusive_ptr.hpp>

#include "custom_object.hpp"
#include "custom_object_type.hpp"
#include "widget.hpp"

namespace gui
{
	class custom_object_widget : public widget
	{
	public:
		custom_object_widget(const variant& v, game_logic::formula_callable* e);
		virtual ~custom_object_widget();
		void set_entity(entity_ptr e);
		entity_ptr get_entity();
		const_entity_ptr get_entity() const;
		void init(const variant& v);
	protected:
		virtual void set_value(const std::string& key, const variant& v);
		virtual variant get_value(const std::string& key) const;

		void handle_draw() const;
		bool handle_event(const SDL_Event& event, bool claimed);
		virtual void handle_process();
	private:
		void click(int button);
		void mouse_enter();
		void mouse_leave();

		boost::function<void (int)> on_click_;
		game_logic::formula_ptr click_handler_;
		boost::function<void ()> on_mouse_enter_;
		game_logic::formula_ptr mouse_enter_handler_;
		boost::function<void ()> on_mouse_leave_;
		game_logic::formula_ptr mouse_leave_handler_;

		game_logic::formula_ptr commands_handler_;

		widget_ptr overlay_;

		entity_ptr entity_;
		bool handle_process_on_entity_;
	};

	typedef boost::intrusive_ptr<custom_object_widget> custom_object_widget_ptr;
	typedef boost::intrusive_ptr<const custom_object_widget> const_custom_object_widget_ptr;
}

#endif
