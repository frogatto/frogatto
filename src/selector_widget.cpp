#include "label.hpp"
#include "foreach.hpp"
#include "image_widget.hpp"
#include "selector_widget.hpp"
#include "widget_factory.hpp"

namespace gui
{
	namespace 
	{
		const std::string selector_left_arrow = "selector_left_arrow";
		const std::string selector_right_arrow = "selector_right_arrow";
	}

	selector_widget::selector_widget(const std::vector<std::string>& list)
		: current_selection_(0)
	{
		set_environment();
		foreach(const std::string& s, list) {
			list_.push_back(selector_pair(s, widget_ptr(new label(s))));
		}
		init();
	}

	selector_widget::selector_widget(const selector_list& list)
		: current_selection_(0), list_(list)
	{
		set_environment();
		init();
	}

	selector_widget::selector_widget(const variant& v, game_logic::formula_callable* e)
		: widget(v, e), current_selection_(v["selection"].as_int(0))
	{
		if(v.has_key("list") || v.has_key("children")) {
			const variant& l = v.has_key("list") ? v["list"] : v["children"];
			ASSERT_LOG(l.is_list(), "'list'/'children' attribute must be a list");
			foreach(const variant& child, l.as_list()) {
				if(child.is_list()) {
					ASSERT_LOG(child.num_elements() == 2, "items in the sub-list must have two elements.");
					widget_ptr w;
					if(child[1].is_map()) {
						w = widget_factory::create(child[1], e);
					} else {
						w = child[1].try_convert<widget>();
						ASSERT_LOG(w != NULL, "Couldn't convert second element to widget.");
					}
					list_.push_back(selector_pair(child[0].as_string(), w));
				} else if(child.is_string()) {
					const std::string& s = child.as_string();
					list_.push_back(selector_pair(s, widget_ptr(new label(s))));
				} else {
					widget_ptr w;
					std::string s;
					if(child.is_map()) {
						w = widget_factory::create(child, e);
						ASSERT_LOG(child.has_key("id") || child.has_key("select_string"), "list items must supply 'id' or 'select_string' attribute.");
						s = child.has_key("id") ? child["id"].as_string() : child["select_string"].as_string();
					} else {
						w = child.try_convert<widget>();
						ASSERT_LOG(w != NULL, "Couldn't convert item to widget.");
						ASSERT_LOG(!w->id().empty(), "list items must have 'id' attribute");
						s = w->id();
					}
					list_.push_back(selector_pair(s, w));
				}
			}
		}

		if(v.has_key("on_change")) {
			change_handler_ = get_environment()->create_formula(v["on_change"]);
			on_change_ = boost::bind(&selector_widget::change_delegate, this, _1);
		}
		if(v.has_key("on_select")) {
			select_handler_ = get_environment()->create_formula(v["on_select"]);
			on_select_ = boost::bind(&selector_widget::select_delegate, this, _1);
		}
		init();
	}

	void selector_widget::set_selection(size_t sel)
	{
		size_t old_sel = current_selection_;
		current_selection_ = sel;
		ASSERT_LOG(current_selection_ < list_.size(), "Selector greater than list size.");
		list_[old_sel].second->enable(false);
		list_[current_selection_].second->enable();
		if(on_change_) {
			on_change_(list_[current_selection_].first);
		}
	}

	void selector_widget::set_selection(const std::string& sel)
	{
		selector_list::iterator it = std::find_if(list_.begin(), list_.end(), 
			boost::bind(&selector_list::value_type::first,_1) == sel);
		ASSERT_LOG(it != list_.end(), "Selection not in list" << sel);
		set_selection(it - list_.begin());
	}

	void selector_widget::init()
	{
		left_arrow_ = widget_ptr(new gui_section_widget(selector_left_arrow));
		right_arrow_ = widget_ptr(new gui_section_widget(selector_right_arrow));

		int width = 16;
		int height = 16;
		int n = 0;
		foreach(const selector_pair& p, list_) {
			if(p.second->width() > width) {
				width = p.second->width();
			}
			if(p.second->height() > height) {
				height = p.second->height();
			}
			if(n != current_selection_) {
				p.second->enable(false);
			} else {
				p.second->enable();
			}
			++n;
		}
		left_arrow_->set_loc(0, (abs(height-left_arrow_->height()))/2);
		//left_arrow_->set_dim(left_arrow_->width(), height);
		right_arrow_->set_loc(left_arrow_->width()+10+width, (abs(height-right_arrow_->height()))/2);
		//right_arrow_->set_dim(right_arrow_->width(), height);
		set_dim(width + left_arrow_->width() + right_arrow_->width() + 10, height);
		for(int n = 0; n != list_.size(); ++n) {
			widget_ptr& w = list_[n].second;
			w->set_loc((width - w->width())/2 + left_arrow_->width()+5, abs(height - w->height())/2);
		}
	}

	void selector_widget::handle_draw() const
	{
		glPushMatrix();
		glTranslatef(GLfloat(x() & ~1), GLfloat(y() & ~1), 0.0);
		if(left_arrow_) {
			left_arrow_->handle_draw();
		}
		if(right_arrow_) {
			right_arrow_->handle_draw();
		}
		if(current_selection_ < list_.size()) {
			if(list_[current_selection_].second) {
				list_[current_selection_].second->handle_draw();
			}
		}
		glPopMatrix();
	}

	bool selector_widget::handle_event(const SDL_Event& event, bool claimed)
	{
		SDL_Event ev = event;
		normalize_event(&ev);

		if(claimed) {
			return claimed;
		}
		if(event.type == SDL_MOUSEMOTION) {
			return handle_mousemotion(event.motion, claimed);
		} else if(event.type == SDL_MOUSEBUTTONDOWN) {
			return handle_mousedown(event.button, claimed);
		} else if(event.type == SDL_MOUSEBUTTONUP) {
			return handle_mouseup(event.button, claimed);
		} else if(event.type == SDL_KEYDOWN) {
#if SDL_VERSION_ATLEAST(2, 0, 0)
			const SDL_Keycode key = event.key.keysym.sym;
#else
			const SDLKey key = event.key.keysym.sym;
#endif
			if(key == SDLK_LEFT || key == SDLK_PAGEUP) {
				select_left();
			} else if(key == SDLK_RIGHT || key == SDLK_PAGEDOWN) {
				select_right();
			} else if(key == SDLK_HOME) {
				if(!list_.empty()) {
					set_selection(0);
				}
			} else if(key == SDLK_END) {
				if(!list_.empty()) {
					set_selection(list_.size()-1);
				}
			} else if(key == SDLK_RETURN && on_select_) {
				on_select_(list_[current_selection_].first);
			}
		}
		return claimed;
	}

	void selector_widget::set_value(const std::string& key, const variant& v)
	{
		if(key == "selection") {
			set_selection(v.as_string());
		}
		widget::set_value(key, v);
	}

	variant selector_widget::get_value(const std::string& key) const
	{
		if(key == "selection") {
			return variant(list_[current_selection_].first);
		} else if(key == "keys") {
			std::vector<variant> v;
			foreach(const selector_pair& p, list_) {
				v.push_back(variant(p.first));
			}
			return variant(&v);
		}
		return widget::get_value(key);
	}

	bool selector_widget::handle_mousedown(const SDL_MouseButtonEvent& event, bool claimed)
	{
		return claimed;
	}

	bool selector_widget::handle_mouseup(const SDL_MouseButtonEvent& event, bool claimed)
	{
		point p(event.x, event.y);
		if(point_in_rect(p, rect(left_arrow_->x(), 
			left_arrow_->y(), 
			left_arrow_->width(), 
			left_arrow_->height()))) {
			select_left();
			claimed = true;
		}
		if(point_in_rect(p, rect(right_arrow_->x(), 
			right_arrow_->y(), 
			right_arrow_->width(), 
			right_arrow_->height()))) {
			select_right();
			claimed = true;
		}
		widget_ptr& cur = list_[current_selection_].second;
		if(point_in_rect(p, rect(cur->x(), cur->y(), cur->width(), cur->height())) && on_select_) {
			on_select_(list_[current_selection_].first);
		}
		return claimed;
	}

	bool selector_widget::handle_mousemotion(const SDL_MouseMotionEvent& event, bool claimed)
	{
		return claimed;
	}

	void selector_widget::change_delegate(const std::string& s)
	{
		if(get_environment()) {
			game_logic::map_formula_callable* callable = new game_logic::map_formula_callable(get_environment());
			callable->add("selection", variant(s));
			callable->add("selected", variant(current_selection_));
			variant v(callable);
			variant value = change_handler_->execute(*callable);
			get_environment()->execute_command(value);
		} else {
			std::cerr << "selector_widget::change_delegate() called without environment!" << std::endl;
		}
	}

	void selector_widget::select_delegate(const std::string& s)
	{
		if(get_environment()) {
			game_logic::map_formula_callable* callable = new game_logic::map_formula_callable(get_environment());
			callable->add("selection", variant(s));
			callable->add("selected", variant(current_selection_));
			variant v(callable);
			variant value = change_handler_->execute(*callable);
			get_environment()->execute_command(value);
		} else {
			std::cerr << "selector_widget::select_delegate() called without environment!" << std::endl;
		}
	}

	void selector_widget::select_left(size_t n)
	{
		int new_sel = int(current_selection_) - int(n);
		while(new_sel < 0) {
			new_sel += int(list_.size());
		}
		set_selection(new_sel);
	}

	void selector_widget::select_right(size_t n)
	{
		set_selection(list_[(current_selection_ + n) % list_.size()].first);
	}
}
