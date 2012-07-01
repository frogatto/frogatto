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
	int nrows() const { return nrows_; }
	void set_min_col_size(int minc) { min_col_size_ = minc; }
	void set_max_col_size(int maxc) { max_col_size_ = maxc; }
	void register_selection_callback(boost::function<void(const variant&, const variant&)> select_fn) 
	{
		on_select_ = select_fn;
	}
	void allow_persistent_highlight(bool val=true, const SDL_Color& col=graphics::color_blue())
	{
		persistent_highlight_ = val;
		highlight_color_ = col;
	}
	variant get_tree() const { return tree_; }
	widget_ptr get_widget_by_id(const std::string& id);
protected:
	virtual void handle_draw() const;
	virtual bool handle_event(const SDL_Event& event, bool claimed);

	virtual void set_value(const std::string& key, const variant& v);
	virtual variant get_value(const std::string& key) const;

	void on_set_yscroll(int old_value, int value);
	virtual void init();
	virtual void on_select(Uint8 button, int selection);

	virtual void on_traverse_element(const variant& key, variant* parent, variant* value, int row);

	int row_height_;
	variant get_selection_key(int selection) const;
	variant tree_;
private:
	virtual int traverse(int depth, int x, int y, variant* parent, const variant& key, variant* value);
	void gen_traverse(int depth, boost::function<void(int,const variant&,variant*)> fn, const variant& key, variant* value);
	int row_at(int xpos, int ypos) const;
	void recalculate_dimensions();
	void calc_column_widths(int depth, const variant& key, variant* value);

	int hpad_;			// Amount int pixels to horizonatally pad
	int col_size_;		// A virtual column
	int font_size_;
	int char_height_;
	int char_width_;
	int min_col_size_;
	int max_col_size_;

	bool allow_selection_;
	bool swallow_clicks_;
	bool must_select_;
	int selected_row_;
	int nrows_;
	int max_height_;

	bool persistent_highlight_;
	SDL_Color highlight_color_;
	int highlighted_row_;

	boost::function<void(const variant&, const variant&)> on_select_;
	std::vector<widget_ptr> widgets_;
	std::map<int, int> last_coords_;
	std::vector<int> col_widths_;
	std::map<int, variant_pair> selection_map_;
};

typedef boost::intrusive_ptr<tree_view_widget> tree_view_widget_ptr;
typedef boost::intrusive_ptr<const tree_view_widget> const_tree_view_widget_ptr;

class tree_editor_widget : public tree_view_widget
{
public:
	explicit tree_editor_widget(int w, int h, const variant& tree);
	explicit tree_editor_widget(const variant& v, game_logic::formula_callable* e);
	virtual ~tree_editor_widget()
	{}
	void set_editor_handler(variant::TYPE vt, widget_ptr editor, boost::function<void(variant*,boost::function<void(const variant&)>)> editor_select) { 
		ex_editor_map_[vt] = editor; 
		on_editor_select_ = editor_select;
	}
	void external_editor_save(variant* v, const variant &new_value);
protected:
	virtual void init();

	virtual void handle_draw() const;
	virtual bool handle_event(const SDL_Event& event, bool claimed);

	virtual void set_value(const std::string& key, const variant& v);
	virtual variant get_value(const std::string& key) const;

	virtual void on_select(Uint8 button, int selection);

	virtual void on_traverse_element(const variant& key, variant* parent, variant* value, int row);
private:
	void context_menu_handler(int tree_selection, const std::vector<std::string>& choices, int menu_selection);
	widget_ptr context_menu_;
	widget_ptr edit_menu_;

	boost::function<void(variant*,boost::function<void(const variant&)>)> on_editor_select_;

	void edit_field(int row, variant* v);
	void execute_edit_enter(const text_editor_widget_ptr editor, variant* value);
	void execute_edit_select(int selection);
	void on_bool_change(variant* v, int selection, const std::string& s);
	void execute_key_edit_enter(const text_editor_widget_ptr editor, variant* parent, const variant& key, variant* value);
	void execute_key_edit_select(int selection);

	std::map<int, std::pair<variant*, variant*> > row_map_;
	std::map<variant::TYPE, widget_ptr> ex_editor_map_;
};

typedef boost::intrusive_ptr<tree_editor_widget> tree_editor_widget_ptr;
typedef boost::intrusive_ptr<const tree_editor_widget> const_tree_editor_widget_ptr;

}

#endif // TREE_VIEW_WIDGET_HPP_INCLUDED
