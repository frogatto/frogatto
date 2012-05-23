#ifndef SCROLLABLE_WIDGET_HPP_INCLUDED
#define SCROLLABLE_WIDGET_HPP_INCLUDED

#include <boost/shared_ptr.hpp>

#include "scrollbar_widget.hpp"
#include "widget.hpp"

namespace gui {

class scrollable_widget : public widget
{
public:
	scrollable_widget();
	scrollable_widget(const variant& v, const game_logic::formula_callable_ptr& e);
	void set_yscroll(int yscroll);
	virtual void set_dim(int w, int h);

	virtual void handle_draw() const;
	virtual bool handle_event(const SDL_Event& event, bool claimed);

	virtual void set_loc(int x, int y);
protected:
	~scrollable_widget();
	void set_virtual_height(int height);
	void set_scroll_step(int step);
	void update_scrollbar();

	int yscroll() const { return yscroll_; }
	int virtual_height() const { return virtual_height_; }

	virtual void set_value(const std::string& key, const variant& v);
	virtual variant get_value(const std::string& key) const;
private:
	virtual void on_set_yscroll(int old_yscroll, int new_yscroll);

	int yscroll_;
	int virtual_height_;
	int step_;

	scrollbar_widget_ptr scrollbar_;
};

typedef boost::intrusive_ptr<scrollable_widget> scrollable_widget_ptr;
typedef boost::intrusive_ptr<const scrollable_widget> const_scrollable_widget_ptr;

}

#endif
