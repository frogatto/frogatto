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
		entity_ptr entity_;
		bool handle_process_on_entity_;
	};

	typedef boost::intrusive_ptr<custom_object_widget> custom_object_widget_ptr;
	typedef boost::intrusive_ptr<const custom_object_widget> const_custom_object_widget_ptr;
}

#endif
