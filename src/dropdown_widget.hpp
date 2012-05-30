#pragma once
#ifndef DROPDOWN_WIDGET_HPP_INCLUDED
#define DROPDOWN_WIDGET_HPP_INCLUDED
#ifndef NO_EDITOR

#include <vector>
#include <boost/intrusive_ptr.hpp>
#include <boost/function.hpp>

#include "border_widget.hpp"
#include "label.hpp"
#include "grid_widget.hpp"
#include "text_editor_widget.hpp"
#include "widget.hpp"

namespace gui {

typedef std::vector<std::string> dropdown_list;

class dropdown_widget : virtual public widget
{
public:
	enum dropdown_type {
		DROPDOWN_LIST,
		DROPDOWN_COMBOBOX,
	};
	dropdown_widget(const dropdown_list& list, int width, int height=0, dropdown_type type=DROPDOWN_LIST);
	dropdown_widget(const variant& v, const game_logic::formula_callable_ptr& e);
	virtual ~dropdown_widget() {}

	void set_on_change_handler(boost::function<void(int,const std::string&)> fn) { on_change_ = fn; }
	void set_selection(int selection);
	int get_max_height() const;
	void set_dropdown_height(int h);
protected:
	virtual void handle_draw() const;
	virtual bool handle_event(const SDL_Event& event, bool claimed);

	virtual void set_value(const std::string& key, const variant& v);
	virtual variant get_value(const std::string& key) const;
	void init();
	void text_enter();
private:
	bool handle_mousedown(const SDL_MouseButtonEvent& event, bool claimed);
	bool handle_mouseup(const SDL_MouseButtonEvent& event, bool claimed);
	bool handle_mousemotion(const SDL_MouseMotionEvent& event, bool claimed);
	void execute_selection(int selection);

	int dropdown_height_;
	dropdown_list list_;
	int current_selection_;
	dropdown_type type_;
	text_editor_widget_ptr editor_;
	grid_ptr dropdown_menu_;
	label_ptr label_;
	widget_ptr dropdown_image_;
	boost::function<void(int, const std::string&)> on_change_;

	// delgate 
	void change_delegate(int selection, const std::string& s);
	// FFL formula
	game_logic::formula_ptr change_handler_;
};

typedef boost::intrusive_ptr<dropdown_widget> dropdown_widget_ptr;
typedef boost::intrusive_ptr<const dropdown_widget> const_dropdown_widget_ptr;

}

#endif // NO_EDITOR
#endif // DROPDOWN_WIDGET_HPP_INCLUDED
