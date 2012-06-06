#pragma once
#ifndef TREE_VIEW_WIDGET_HPP_INCLUDED
#define TREE_VIEW_WIDGET_HPP_INCLUDED

#include <map>
#include <vector>
#include <boost/bind.hpp>
#include <boost/function.hpp>

#include "scrollable_widget.hpp"
#include "widget.hpp"
#include "variant.hpp"

namespace gui {

class tree_view_widget : public virtual scrollable_widget
{
public:
	explicit tree_view_widget(int w, int h, const variant& tree);
	explicit tree_view_widget(const variant& v, game_logic::formula_callable* e);
	virtual ~tree_view_widget()
	{}
	void allow_selection(bool val=true) { allow_selection_ = val; }
	void must_select(bool val=true, int nrow=0) { must_select_ = val; selected_row_ = nrow; }
	bool has_must_select() const { return must_select_; }
	void swallow_clicks(bool val=true) { swallow_clicks_ = val; }
	int selection() const { return selected_row_; }
	void register_selection_callback(boost::function<void(int,std::pair<std::string, variant*>)> cb) { on_select_ = cb; }
	int nrows() const { return nrows_; }
protected:
	virtual void handle_draw() const;
	virtual bool handle_event(const SDL_Event& event, bool claimed);

	virtual void set_value(const std::string& key, const variant& v);
	virtual variant get_value(const std::string& key) const;

	void on_set_yscroll(int old_value, int value);
private:
	int traverse(int x, int y, const variant& key, variant* value);
	int row_at(int xpos, int ypos) const;
	void recalculate_dimensions();

	variant tree_;

	int hpad_;			// Amount int pixels to horizonatally pad
	int col_size_;		// A virtual column
	int font_size_;
	int char_height_;

	bool allow_selection_;
	bool swallow_clicks_;
	bool must_select_;
	int selected_row_;
	int row_height_;
	int nrows_;
	int max_height_;

	boost::function<void(int,std::pair<std::string, variant*>)> on_select_;

	std::vector<widget_ptr> widgets_;
	std::map<int, int> last_coords_;
	std::map<int, std::pair<std::string, variant*> > row_map_;
};

typedef boost::intrusive_ptr<tree_view_widget> tree_view_widget_ptr;
typedef boost::intrusive_ptr<const tree_view_widget> const_tree_view_widget_ptr;

}

#endif // TREE_VIEW_WIDGET_HPP_INCLUDED
