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
#include <algorithm>

#include "foreach.hpp"
#include "label.hpp"
#include "rich_text_label.hpp"
#include "string_utils.hpp"
#include "variant_utils.hpp"
#include "widget_factory.hpp"

namespace gui
{

namespace {
void flatten_recursively(const std::vector<variant>& v, std::vector<variant>* result)
{
	foreach(const variant& item, v) {
		if(item.is_list()) {
			flatten_recursively(item.as_list(), result);
		} else {
			result->push_back(item);
		}
	}
}

}

rich_text_label::rich_text_label(const variant& v, game_logic::formula_callable* e)
	: widget(v,e)
{
	children_.resize(1);
	children_.front().clear();

	int xpos = 0, ypos = 0;
	int line_height = 0;
	std::vector<variant> items;
	flatten_recursively(v["children"].as_list(), &items);
	foreach(const variant& item, items) {
		const std::string widget_type = item["type"].as_string();
		if(widget_type == "label") {
			const std::vector<std::string> lines = util::split(item["text"].as_string(), '\n', 0);

			variant label_info = deep_copy_variant(item);

			for(int n = 0; n != lines.size(); ++n) {
				if(n != 0) {
					xpos = 0;
					ypos += line_height;
					line_height = 0;
					children_.resize(children_.size()+1);
				}

				std::string candidate;
				std::string line = lines[n];
				while(!line.empty()) {
					std::string::iterator space_itor = std::find(line.begin()+1, line.end(), ' ');

					std::string words(line.begin(), space_itor);
					label_info.add_attr_mutation(variant("text"), variant(words));
					widget_ptr label_widget_holder(widget_factory::create(label_info, e));
					label_ptr label_widget(static_cast<label*>(label_widget_holder.get()));

					bool skip_leading_space = false;

					if(xpos != 0 && xpos + label_widget->width() > width()) {
						xpos = 0;
						ypos += line_height;
						line_height = 0;
						skip_leading_space = true;
						children_.resize(children_.size()+1);
					}


					candidate = words;

					while(xpos + label_widget->width() < width() && space_itor != line.end()) {
						candidate = words;
						
						space_itor = std::find(space_itor+1, line.end(), ' ');

						words = std::string(line.begin(), space_itor);
						label_widget->set_text(words);
					}

					line.erase(line.begin(), line.begin() + candidate.size());
					if(skip_leading_space && candidate.empty() == false && candidate[0] == ' ') {
						candidate.erase(candidate.begin());
					}
					label_widget->set_text(candidate);
					label_widget->set_loc(xpos, ypos);

					if(label_widget->height()*0.75 > line_height) {
						line_height = label_widget->height()*0.75;
					}

					xpos += label_widget->width();

					children_.back().push_back(label_widget);
				}
			}
		} else {
			//any widget other than a label
			widget_ptr w(widget_factory::create(item, e));

			if(xpos != 0 && xpos + w->width() > width()) {
				xpos = 0;
				ypos += line_height;
				line_height = 0;
				children_.resize(children_.size()+1);
			}

			if(w->height() > line_height) {
				line_height = w->height();
			}

			w->set_loc(xpos, ypos);

			xpos += w->width();

			children_.back().push_back(w);
		}
	}

	if(v["align"].as_string_default("left") == "right") {
		foreach(std::vector<widget_ptr>& v, children_) {
			if(!v.empty()) {
				const int delta = x() + width() - (v.back()->x() + v.back()->width());
				foreach(widget_ptr w, v) {
					w->set_loc(w->x() + delta, w->y());
				}
			}
		}
	}

	if(v["valign"].as_string_default("center") == "center") {
		foreach(std::vector<widget_ptr>& v, children_) {
			if(!v.empty()) {
				int height = 0;
				foreach(const widget_ptr& w, v) {
					if(w->height() > height) {
						height = w->height();
					}
				}

				foreach(const widget_ptr& w, v) {
					if(w->height() < height) {
						w->set_loc(w->x(), w->y() + (height - w->height())/2);
					}
				}
			}
		}
	}

	set_dim(width(), ypos + line_height);

	set_claim_mouse_events(v["claim_mouse_events"].as_bool(false));
}

void rich_text_label::handle_process()
{
	foreach(const std::vector<widget_ptr>& v, children_) {
		foreach(const widget_ptr& widget, v) {
			widget->process();
		}
	}
}

void rich_text_label::handle_draw() const
{
	glPushMatrix();
	glTranslatef(x() & ~1, y() & ~1, 0.0);

	foreach(const std::vector<widget_ptr>& v, children_) {
		foreach(const widget_ptr& widget, v) {
			widget->draw();
		}
	}

	glPopMatrix();
}

bool rich_text_label::handle_event(const SDL_Event& event, bool claimed)
{
	claimed = scrollable_widget::handle_event(event, claimed);

	SDL_Event ev = event;
	normalize_event(&ev);
	foreach(const std::vector<widget_ptr>& v, children_) {
		foreach(const widget_ptr& widget, v) {
			claimed = widget->process_event(ev, claimed);
		}
	}

	return claimed;
}

variant rich_text_label::get_value(const std::string& key) const
{
	return widget::get_value(key);
}

void rich_text_label::set_value(const std::string& key, const variant& v)
{
	widget::set_value(key, v);
}

}
