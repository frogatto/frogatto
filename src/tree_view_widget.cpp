#ifndef NO_EDITOR

#include <algorithm>
#include <sstream>

#include <boost/algorithm/string/replace.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>

#include "dropdown_widget.hpp"
#include "filesystem.hpp"
#include "font.hpp"
#include "foreach.hpp"
#include "grid_widget.hpp"
#include "label.hpp"
#include "poly_line_widget.hpp"
#include "raster.hpp"
#include "text_editor_widget.hpp"
#include "tree_view_widget.hpp"

namespace gui {

tree_view_widget::tree_view_widget(int w, int h, const variant& tree)
	: tree_(tree), hpad_(10), col_size_(80), font_size_(12), selected_row_(-1), 
	char_height_(font::char_height(font_size_)), allow_selection_(false),
	must_select_(false), nrows_(0), swallow_clicks_(false),
	max_height_(-1), min_col_size_(20), max_col_size_(80), char_width_(font::char_width(font_size_)),
	persistent_highlight_(false), 
	highlight_color_(graphics::color_blue()), highlighted_row_(-1)
{
	row_height_ = font::char_height(font_size_);
	set_environment();
	widget::set_dim(w, h);
	init();
}

tree_view_widget::tree_view_widget(const variant& v, game_logic::formula_callable* e)
	: scrollable_widget(v,e), selected_row_(-1), nrows_(0), min_col_size_(20), max_col_size_(80),
	persistent_highlight_(false), highlight_color_(graphics::color_blue()), highlighted_row_(-1)
{
	tree_ = v["child"];

	hpad_ = v["horizontal_padding"].as_int(10);
	col_size_ = v["column_size"].as_int(80);
	font_size_ = v["font_size"].as_int(12);
	allow_selection_ = v["allow_selection"].as_bool(false);
	must_select_ = v["must_select"].as_bool(false);
	max_height_ = v["max_height"].as_int(-1);

	char_height_ = font::char_height(font_size_);
	char_width_ = font::char_width(font_size_);
	row_height_ = font::char_height(font_size_);

	init();
}

void tree_view_widget::init()
{
	ASSERT_LOG(tree_.is_map() == true, "Tree passed to the tree_view_widget must be a map object.");

	col_widths_.clear();
	col_widths_.push_back(col_size_/2);
	gen_traverse(0, boost::bind(&tree_view_widget::calc_column_widths, this, _1, _2, _3), variant(), &tree_);

	std::cerr << "Column widths: ";
	foreach(int colw, col_widths_) {
		std::cerr << colw << ", ";
	}
	std::cerr << std::endl;

	selection_map_.clear();
	widgets_.clear();
	last_coords_.clear();
	nrows_ = 0;
	traverse(0, col_size_/2, 0, &tree_, variant(), &tree_);
	recalculate_dimensions();
}

int tree_view_widget::traverse(int depth, int x, int y, variant* parent, const variant& key, variant* value)
{
	std::vector<point> points;
	std::map<int,int>::iterator it = last_coords_.find(x);
	int last_y = y;
	if(it != last_coords_.end()) {
		last_y = it->second;
	}
	points.push_back(point(x/2, last_y));
	points.push_back(point(x/2, y+char_height_/2));
	points.push_back(point(x, y+char_height_/2 ));
	poly_line_widget_ptr plw(new poly_line_widget(&points, graphics::color_grey()));
	widgets_.push_back(plw);
	last_coords_[x] = y + char_height_/2;

	label_ptr key_label;
	if(key.is_null() == false) {
		std::string str(key.as_string());
		// list or map don't need to trunate the key.
		if(!value->is_list() && !value->is_map()) {
			int max_chars = col_widths_[depth]/char_width_;
			if(str.length() > max_chars && max_chars > 3) {
				str = str.substr(0, max_chars-3) + "...";
			}
		}
		key_label.reset(new label(str, graphics::color_white(), font_size_));
		key_label->set_loc(x, y);
		key_label->set_dim(col_widths_[depth], key_label->height());
		x += col_widths_[depth] + hpad_;
		widgets_.push_back(key_label);
	}
	if(value->is_null()) {
		label_ptr null_label(new label("<null>", graphics::color_yellow(), font_size_));
		null_label->set_loc(x, y);
		null_label->set_dim(col_widths_[depth], null_label->height());
		widgets_.push_back(null_label);
		y += widgets_.back()->height();
	} else if(value->is_int()) {
		std::stringstream ss;
		ss << value->as_int();
		label_ptr int_label(new label(ss.str(), graphics::color_yellow(), font_size_));
		int_label->set_loc(x, y);
		int_label->set_dim(col_widths_[depth], int_label->height());
		widgets_.push_back(int_label);
		y += widgets_.back()->height();
	} else if(value->is_decimal()) {
		std::stringstream ss;
		ss << value->as_decimal();
		label_ptr decimal_label(new label(ss.str(), graphics::color_yellow(), font_size_));
		decimal_label->set_loc(x, y);
		decimal_label->set_dim(col_widths_[depth], decimal_label->height());
		widgets_.push_back(decimal_label);
		y += widgets_.back()->height();
	} else if(value->is_string()) {
		std::string str(value->as_string());
		boost::replace_all(str, "\n", "\\n");
		int max_chars = (width()-x)/char_width_;
		if(str.length() > max_chars && max_chars > 3) {
			str = str.substr(0, max_chars-3) + "...";
		}
		label_ptr string_label(new label(str, graphics::color_yellow(), font_size_));
		string_label->set_loc(x, y);
		string_label->set_dim(col_widths_[depth], string_label->height());
		widgets_.push_back(string_label);
		y += widgets_.back()->height();
	} else if(value->is_list()) {
		if(key_label) {
			y += key_label->height();
			on_traverse_element(key, parent, value, nrows_);
		}
		
		for(int index = 0; index != value->as_list().size(); index++) {
			variant* item = value->get_index_mutable(index);
			y = traverse(depth+1, x, y, value, item->is_map() ? variant("<map>") : variant(), item);
		}
		last_coords_.erase(x);
	} else if(value->is_map()) {
		if(key_label) {
			y += key_label->height();
			on_traverse_element(key, parent, value, nrows_);
		}
		foreach(const variant& k, value->get_keys().as_list()) {
			y = traverse(depth+1, x, y, value, k, value->get_attr_mutable(k));
		}
		last_coords_.erase(x);
	} else if(value->is_bool()) {
		label_ptr bool_label(new label(value->as_bool() ? "true" : "false", graphics::color_yellow(), font_size_));
		bool_label->set_loc(x, y);
		bool_label->set_dim(col_widths_[depth], bool_label->height());
		widgets_.push_back(bool_label);
		y += widgets_.back()->height();
	}
	if(!value->is_map() && !value->is_list()) {
		on_traverse_element(key, parent, value, nrows_);
	}
	return y;
}

void tree_view_widget::gen_traverse(int depth, 
	boost::function<void(int,const variant&,variant*)> fn, 
	const variant& key, 
	variant* value)
{
	if(value->is_map()) {
		foreach(const variant& k, value->get_keys().as_list()) {
			gen_traverse(depth + 1, fn, k, value->get_attr_mutable(k));
		}
	} else if(value->is_list()) {
		for(int index = 0; index != value->as_list().size(); index++) {
			gen_traverse(depth + 1, fn, variant(), value->get_index_mutable(index));
		}
	} else if(value->is_string() || value->is_null() || value->is_int() || value->is_decimal() || value->is_bool()) {
		// skip so we just call fn() at the end.
	} else {
		std::cerr << "gen_traverse(): Ignored variant element: " << value->to_debug_string() << std::endl;
		return;
	}
	fn(depth, key, value);
}

void tree_view_widget::calc_column_widths(int depth, const variant& key, variant* value)
{
	while(col_widths_.size() <= depth) {
		col_widths_.push_back(min_col_size_);
	}
	int str_chars = 0;
	if(value->is_string()) {
		str_chars = value->as_string().length();
	} else if(value->is_numeric()) {
		std::stringstream ss;
		ss << *value;
		str_chars = ss.str().length();
		//std::cerr << "XXX: " << *value << ", " << str_chars << ", " << (str_chars*char_width_) << std::endl;
	} else if(value->is_null()) {
		str_chars = 6;  // "<null>"
	} else if(value->is_bool()) {
		str_chars = 5;	// length of "false".
	} else {
		str_chars = 999;	// Arbitrarily large value
	}
	if(key.is_null() == false) {
		if(int(key.as_string().length()*char_width_) > col_widths_[depth-1]) {
			col_widths_[depth-1] = key.as_string().length()*char_width_;
			if(col_widths_[depth-1] > max_col_size_) {
				col_widths_[depth-1] = max_col_size_;
			}
		}
		if(col_widths_[depth-1] < min_col_size_) {
			col_widths_[depth-1] = min_col_size_;
		}
		//col_widths_[depth-1] = std::min(min_col_size_, std::max(int(key.as_string().length()*char_width_), std::max(max_col_size_, col_widths_[depth-1])));
	}
	//col_widths_[depth] = std::min(min_col_size_, std::max(str_chars*char_width_, std::max(max_col_size_, col_widths_[depth])));
	if(str_chars*char_width_ > col_widths_[depth]) {
		col_widths_[depth] = str_chars*char_width_;
		if(col_widths_[depth] > max_col_size_) {
			col_widths_[depth] = max_col_size_;
		}
	}
	if(col_widths_[depth] < min_col_size_) {
		col_widths_[depth] = min_col_size_;
	}
}

void tree_view_widget::on_traverse_element(const variant& key, variant* parent, variant* value, int row)
{
	//std::cerr << "on_traverse_element() " << row << " : " << (key.is_null() ? "<null key>" : key.as_string()) 
	//	<< " : value(" << value->to_debug_string() << ")" << std::endl;
	selection_map_[nrows_] = variant_pair(key, *value);
	nrows_++;
}

variant tree_view_widget::get_selection_key(int selection) const
{
	std::map<int, variant_pair>::const_iterator it = selection_map_.find(selection);
	if(it == selection_map_.end()) {
		std::cerr << "Key not found for selection. " << selection << std::endl;
		return variant("");
	}
	return it->second.first;
}

void tree_view_widget::handle_draw() const
{
	graphics::draw_hollow_rect(
		rect(x(), y(), width(), height()).sdl_rect(), 
		graphics::color_grey());

	glPushMatrix();
	glTranslatef(GLfloat(x()+2 & ~1), GLfloat(y()+2 & ~1), 0.0);

	if(selected_row_ >= 0 && selected_row_ < nrows()) {
		SDL_Rect rect = {0,row_height_*selected_row_ - yscroll(),width(),row_height_};
		const SDL_Color col = {0xFF,0x00,0x00,0x00};
		graphics::draw_rect(rect,col,128);
	}

	if(persistent_highlight_ && highlighted_row_ >= 0 && size_t(persistent_highlight_) < nrows()) {
		SDL_Rect rect = {0,row_height_*highlighted_row_ - yscroll(),width(),row_height_};
		graphics::draw_rect(rect, highlight_color_, 128);
	}
	glTranslatef(0, GLfloat(-yscroll() & ~1), 0.0);

	foreach(const widget_ptr& w, widgets_) {
		w->draw();
	}
	glPopMatrix();

	scrollable_widget::handle_draw();
}

int tree_view_widget::row_at(int xpos, int ypos) const
{
	if(row_height_ == 0) {
		return -1;
	} else if(xpos > x()+1 && xpos < x()-1 + width() &&
	   ypos > y()+1 && ypos < y()-1 + height()) {
		return (ypos + yscroll() - y()-1) / row_height_;
	} else {
		return -1;
	}
}

void tree_view_widget::recalculate_dimensions()
{
	int desired_height = row_height_*nrows();
	set_virtual_height(desired_height);
	set_scroll_step(row_height_);
	
	if(max_height_ > 0 && desired_height > max_height_) {
		desired_height = max_height_;
		while(desired_height%row_height_) {
			--desired_height;
		}
	}

	foreach(const widget_ptr& w, widgets_) {
		if(w->y() - yscroll() >= 0 && w->y() + w->height() - yscroll() < height()+2) {
			w->set_visible(true);
		} else {
			w->set_visible(false);
		}
	}
	
	update_scrollbar();
}

void tree_view_widget::on_set_yscroll(int old_value, int value)
{
	recalculate_dimensions();
}


bool tree_view_widget::handle_event(const SDL_Event& event, bool claimed)
{
	claimed = scrollable_widget::handle_event(event, claimed);

	rect r(x(), y(), width(), height());
	if(!claimed && allow_selection_) {
		if(event.type == SDL_MOUSEMOTION) {
			const SDL_MouseMotionEvent& e = event.motion;
			if(point_in_rect(point(e.x, e.y), r)) {
				int new_row = row_at(e.x,e.y);
				if(new_row != selected_row_) {
					selected_row_ = new_row;
				}
			}
		} else if(event.type == SDL_MOUSEBUTTONDOWN) {
			point p(event.button.x, event.button.y);
			if(event.button.button == SDL_BUTTON_WHEELUP && point_in_rect(p, r)) {
				set_yscroll(yscroll() - 3*row_height_ < 0 ? 0 : yscroll() - 3*row_height_);
				selected_row_ -= 3;
				if(selected_row_ < 0) {
					selected_row_ = 0;
				}
				claimed = true;
			} else if(event.button.button == SDL_BUTTON_WHEELDOWN  && point_in_rect(p, r)) {
				int y3 = yscroll() + 3*row_height_;
				set_yscroll(virtual_height() - y3 < height() 
					? virtual_height() - height()
					: y3);
				selected_row_ += 3;
				if(selected_row_ >= nrows()) {
					selected_row_ = nrows() - 1;
				}
				claimed = true;
			} else {
				const SDL_MouseButtonEvent& e = event.button;
				const int row_index = row_at(e.x, e.y);
				on_select(e.button, row_index);
			}
			if(swallow_clicks_) {
				claimed = true;
			}
		}
	}

	if(!claimed && must_select_) {
		if(event.type == SDL_KEYDOWN) {
			if(event.key.keysym.sym == SDLK_UP) {
				if(selected_row_-- == 0) {
					selected_row_ = nrows()-1;
				}
				claimed = true;
			} else if(event.key.keysym.sym == SDLK_DOWN) {
				if(++selected_row_ == nrows()) {
					selected_row_ = 0;
				}
				claimed = true;
			}
		}
	}

	SDL_Event ev = event;
	normalize_event(&ev);
	reverse_foreach(const widget_ptr& widget, widgets_) {
		if(widget) {
			if(widget->process_event(ev, claimed)) {
				return true;
			}
		}
	}

	return claimed;
}

void tree_view_widget::on_select(Uint8 button, int selection)
{
	if(persistent_highlight_) {
		highlighted_row_ = selection;
	}

	if(button == SDL_BUTTON_LEFT) {
		std::cerr << "TREEVIEW SELECT ROW(edit): " << selection << "\n";
	if(on_select_) {
		on_select_(selection_map_[selection].first, selection_map_[selection].second);
	}
	} else if(button == SDL_BUTTON_RIGHT) {
		std::cerr << "TREEVIEW SELECT ROW(context): " << selection << "\n";
	}
}

widget_ptr tree_view_widget::get_widget_by_id(const std::string& id)
{
	foreach(const widget_ptr& w, widgets_) {
		widget_ptr wx = w->get_widget_by_id(id);
		if(wx) {
			return wx;
		}
	}
	return widget::get_widget_by_id(id);
}

void tree_view_widget::set_value(const std::string& key, const variant& v)
{
	widget::set_value(key, v);
}

variant tree_view_widget::get_value(const std::string& key) const
{
	return widget::get_value(key);
}

tree_editor_widget::tree_editor_widget(int w, int h, const variant& tree)
	: tree_view_widget(w,h,tree)
{
	init();
}

tree_editor_widget::tree_editor_widget(const variant& v, game_logic::formula_callable* e)
	: tree_view_widget(v,e)
{
}

void tree_editor_widget::on_traverse_element(const variant& key, variant* parent, variant* value, int row)
{
	row_map_[row] = std::pair<variant*, variant*>(parent, value);
	tree_view_widget::on_traverse_element(key, parent, value, row);
}

void tree_editor_widget::handle_draw() const
{
	tree_view_widget::handle_draw();

	if(context_menu_) {
		context_menu_->draw();
	}
	if(edit_menu_) {
		edit_menu_->draw();
	}
}

bool tree_editor_widget::handle_event(const SDL_Event& event, bool claimed)
{
	if(edit_menu_ && edit_menu_->process_event(event, claimed)) {
		return true;
	}

	if(context_menu_ && context_menu_->process_event(event, claimed)) {
		return true;
	}

	if(claimed || tree_view_widget::handle_event(event, claimed)) {
		return true;
	}
	return claimed;
}

void tree_editor_widget::init()
{
	row_map_.clear();
	context_menu_.reset();
	edit_menu_.reset();
	tree_view_widget::init();
	if(tree_.is_map() && tree_.num_elements() == 0) {
		row_map_[0] = std::pair<variant*, variant*>((variant*)NULL, &tree_);
	}
}

void tree_editor_widget::on_select(Uint8 button, int selection)
{
	if(button == SDL_BUTTON_RIGHT) {
		if(selection != -1) {
			std::map<int, std::pair<variant*, variant*> >::const_iterator it 
				= row_map_.find(selection);
			if(it == row_map_.end()) {
				selection = row_map_.end()->first;
			}
			variant* v = row_map_[selection].second;
			variant* parent_container = row_map_[selection].first;

			// Create a menu to select how to edit the item.
			gui::grid* g = new gui::grid(1);
			g->set_show_background(true);
			g->allow_selection(true);
			g->swallow_clicks(true);
			g->allow_draw_highlight(true);
			std::vector<std::string> choices;
			if(parent_container != NULL) {
				choices.push_back("Edit");
				if(parent_container && parent_container->is_map()) {
					choices.push_back("Edit Key");
				}
				choices.push_back("----------------");
				choices.push_back("Edit As: Integer");
				choices.push_back("Edit As: Decimal");
				choices.push_back("Edit As: Boolean");
				choices.push_back("Edit As: String");
				choices.push_back("----------------");
			}
			choices.push_back("Add Integer");
			choices.push_back("Add Decimal");
			choices.push_back("Add Boolean");
			choices.push_back("Add String");
			choices.push_back("Add List");
			choices.push_back("Add Map");
			if(parent_container != NULL) {
				choices.push_back("----------------");
				choices.push_back("Delete");
			}

			foreach(const std::string& str, choices) {
				g->add_col(label_ptr(new label(str)));
			}
			g->register_selection_callback(boost::bind(&tree_editor_widget::context_menu_handler, this, selection, choices, _1));
			int mousex, mousey;
			SDL_GetMouseState(&mousex, &mousey);
			mousex -= x();
			mousey -= y();
			context_menu_.reset(g);
			int posy = y() + row_height_ * selection - yscroll();
			if(posy + g->height() > y() + height()) {
				posy = y() + height() - posy + g->height();
			}
			context_menu_->set_loc(mousex, posy);
		}
	}
	tree_view_widget::on_select(button, selection);
}

void tree_editor_widget::context_menu_handler(int tree_selection, const std::vector<std::string>& choices, int menu_selection)
{
	if(menu_selection < 0 || size_t(menu_selection) >= choices.size()) {
		if(context_menu_) {
			context_menu_.reset();
		}
		return;
	}
	std::cerr << "Tree selection: " << tree_selection << std::endl;
	
	// Menu seperators have a '-' character in the first position.
	if(choices[menu_selection][0] == '-') {
		return;
	}

	variant* v = row_map_[tree_selection].second;
	variant* parent_container = row_map_[tree_selection].first;

	if(choices[menu_selection] == "Delete") {
		if(parent_container->is_map()) {
			parent_container->remove_attr(get_selection_key(tree_selection));
		} else if(parent_container->is_list()) {
			std::vector<variant> new_list = parent_container->as_list();
			new_list.erase(std::remove(new_list.begin(), new_list.end(), *v), new_list.end());
		}
		init();
	} else if(choices[menu_selection] == "Edit Key") {
		gui::grid* grid = new gui::grid(1);
		grid->set_show_background(true);
		grid->allow_selection(true);
		grid->swallow_clicks(false);
		grid->allow_draw_highlight(false);
		text_editor_widget_ptr editor = new text_editor_widget(200, 28);
		editor->set_font_size(14);
		editor->set_on_enter_handler(boost::bind(&tree_editor_widget::execute_key_edit_enter, this, editor, parent_container, get_selection_key(tree_selection), v));
		editor->set_on_tab_handler(boost::bind(&tree_editor_widget::execute_key_edit_enter, this, editor, parent_container, get_selection_key(tree_selection), v));
		editor->set_on_esc_handler(boost::bind(&tree_editor_widget::init, this));
		editor->set_text(get_selection_key(tree_selection).as_string());
		editor->set_focus(true);
		grid->add_col(editor);
		grid->register_selection_callback(boost::bind(&tree_editor_widget::execute_key_edit_select, this, _1));
		int mousex, mousey;
		SDL_GetMouseState(&mousex, &mousey);
		mousex -= x();
		mousey -= y();
		edit_menu_.reset(grid);
		edit_menu_->set_loc(mousex, y() + row_height_ * tree_selection - yscroll());
	} else if(choices[menu_selection].substr(0, 4) == "Edit") {
		std::string choice_type = choices[menu_selection].length() > 4 ? choices[menu_selection].substr(9) : "";
		if(choice_type == "Integer") {
			*v = variant(0);
		} else if(choice_type == "Decimal") {
			*v = variant(0.0);
		} else if(choice_type == "Boolean") {
			*v = variant::from_bool(false);
		} else if(choice_type == "String") {
			*v = variant("");
		}
		edit_field(tree_selection, v);
	} else if(choices[menu_selection].substr(0, 4) == "Add ") {
		if(v->is_list() || v->is_map()) {
			parent_container = v;
		}
		std::string choice_type = choices[menu_selection].length() > 4 ? choices[menu_selection].substr(4) : "";
		std::vector<variant> new_list;
		if(choice_type == "Integer") {
			if(parent_container->is_map()) {
				parent_container->add_attr(variant("a_new_integer"), variant(0));
			} else {
				new_list.push_back(variant(0));
				*parent_container = *parent_container + variant(&new_list);
			}
		} else if(choice_type == "Decimal") {
			if(parent_container->is_map()) {
				parent_container->add_attr(variant("a_new_decimal"), variant(0.0));
			} else {
				new_list.push_back(variant(0.0));
				*parent_container = *parent_container + variant(&new_list);
			}
		} else if(choice_type == "Boolean") {
			if(parent_container->is_map()) {
				parent_container->add_attr(variant("a_new_boolean"), variant::from_bool(false));
			} else {
				new_list.push_back(variant::from_bool(false));
				*parent_container = *parent_container + variant(&new_list);
			}
		} else if(choice_type == "String") {
			if(parent_container->is_map()) {
				parent_container->add_attr(variant("a_new_string"), variant(""));
			} else {
				new_list.push_back(variant("a string"));
				*parent_container = *parent_container + variant(&new_list);
			}
		} else if(choice_type == "List") {
			std::vector<variant> inner_list;
			inner_list.push_back(variant("a string"));
			new_list.push_back(variant(&inner_list));
			if(parent_container->is_map()) {
				parent_container->add_attr(variant("a_new_list"), variant(&new_list));
			} else {
				*parent_container = *parent_container + variant(&new_list);
			}
		} else if(choice_type == "Map") {
			std::map<variant, variant> new_map;
			new_map[variant("a_key")] = variant("a string");
			if(parent_container->is_map()) {
				parent_container->add_attr(variant("a_new_map"), variant(&new_map));
			} else {
				new_list.push_back(variant(&new_map));
				*parent_container = *parent_container + variant(&new_list);
			}
		}
		init();
	}
}

void tree_editor_widget::edit_field(int row, variant* v)
{
	if(context_menu_) {
		context_menu_.reset();
	}
	if(edit_menu_) {
		edit_menu_.reset();
	}

	std::map<variant::TYPE, widget_ptr>::iterator it = ex_editor_map_.find(v->type());
	if(it != ex_editor_map_.end()) {
		if(on_editor_select_) {
		    on_editor_select_(v, boost::bind(&tree_editor_widget::external_editor_save, this, v, _1));
		}
		edit_menu_ = it->second;
		return;
	}

	if(v->is_map() 
		|| v->is_list() 
		|| v->is_function() 
		|| v->is_callable() 
		|| v->is_null()) {
		return;
	}

	gui::grid* grid = new gui::grid(1);
	grid->set_show_background(true);
	grid->allow_selection(true);
	grid->swallow_clicks(false);
	grid->allow_draw_highlight(false);

	if(v->is_numeric() || v->is_string()) {
		text_editor_widget_ptr editor = new text_editor_widget(200, 28);
		editor->set_font_size(14);
		editor->set_on_enter_handler(boost::bind(&tree_editor_widget::execute_edit_enter, this, editor, v));
		editor->set_on_tab_handler(boost::bind(&tree_editor_widget::execute_edit_enter, this, editor, v));
		editor->set_on_esc_handler(boost::bind(&tree_editor_widget::init, this));
		std::stringstream ss;
		if(v->is_int()) {
			ss << v->as_int();
		} else if(v->is_decimal()) {
			ss << v->as_decimal();
		} else if(v->is_string()) {
			ss << v->as_string();
		}
		editor->set_text(ss.str());
		editor->set_focus(true);
		grid->add_col(editor);
	} else if(v->is_bool()) {
		std::vector<std::string> bool_list;
		bool_list.push_back("false");
		bool_list.push_back("true");
		dropdown_widget_ptr bool_dd(new dropdown_widget(bool_list, 100, 30));
		bool_dd->set_selection(v->as_bool());
		bool_dd->set_on_select_handler(boost::bind(&tree_editor_widget::on_bool_change, this, v, _1, _2));
		grid->add_col(bool_dd);
	}
	grid->register_selection_callback(boost::bind(&tree_editor_widget::execute_edit_select, this, _1));

	int mousex, mousey;
	SDL_GetMouseState(&mousex, &mousey);
	mousex -= x();
	mousey -= y();
	edit_menu_.reset(grid);
	edit_menu_->set_loc(mousex, y() + row_height_ * row - yscroll());
}

void tree_editor_widget::execute_edit_enter(const text_editor_widget_ptr editor, variant* value)
{
	if(edit_menu_) {
		edit_menu_.reset();
	}

	if(editor->text().empty() == false) {
		std::stringstream ss(editor->text());
		if(value->is_int()) {
			int i;
			ss >> i;
			*value = variant(i);
		} else if(value->is_decimal()) {
			double f;
			ss >> f;
			*value = variant(f);
		} else if(value->is_string()) {
			*value = variant(editor->text());
		}
	}
	init();
}

void tree_editor_widget::execute_key_edit_enter(const text_editor_widget_ptr editor, variant* parent, const variant& key, variant* value)
{
	if(editor->text().empty() == false) {
		if(edit_menu_) {
			edit_menu_.reset();
		}
		parent->add_attr(variant(editor->text()), (*parent)[key]);
		parent->remove_attr(key);

		init();
	}
}

void tree_editor_widget::execute_key_edit_select(int selection)
{
	if(selection == -1 && edit_menu_) {
		edit_menu_.reset();
	}
}

void tree_editor_widget::external_editor_save(variant* v, const variant &new_value)
{
	if(edit_menu_) {
		edit_menu_.reset();
	}
	*v = new_value;
	init();
}

void tree_editor_widget::execute_edit_select(int selection)
{
	if(selection == -1 && edit_menu_) {
		edit_menu_.reset();
	}
}

void tree_editor_widget::on_bool_change(variant* v, int selection, const std::string& s)
{
	if(edit_menu_) {
		edit_menu_.reset();
	}

	if(selection < 0 || selection > 1) {
		return;
	}
	*v = variant::from_bool(selection);
	init();
}

void tree_editor_widget::set_value(const std::string& key, const variant& v)
{
	tree_view_widget::set_value(key, v);
}

variant tree_editor_widget::get_value(const std::string& key) const
{
	return tree_view_widget::get_value(key);
}

}

#endif
