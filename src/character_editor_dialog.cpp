#ifndef NO_EDITOR
#include <boost/bind.hpp>

#include "border_widget.hpp"
#include "button.hpp"
#include "character_editor_dialog.hpp"
#include "editor.hpp"
#include "foreach.hpp"
#include "frame.hpp"
#include "grid_widget.hpp"
#include "image_widget.hpp"
#include "label.hpp"
#include "module.hpp"
#include "raster.hpp"
#include "text_editor_widget.hpp"

namespace editor_dialogs
{

character_editor_dialog::character_editor_dialog(editor& e)
  : gui::dialog(graphics::screen_width() - EDITOR_SIDEBAR_WIDTH, 160, EDITOR_SIDEBAR_WIDTH, 440), editor_(e)
{
	set_clear_bg_amount(255);
	if(editor_.all_characters().empty() == false) {
		category_ = editor_.all_characters().front().category;
	}

	init();
}

void character_editor_dialog::init()
{
	clear();
	using namespace gui;
	set_padding(20);

	if(!find_edit_) {
		find_edit_.reset(new text_editor_widget(140));
		find_edit_->set_on_change_handler(boost::bind(&character_editor_dialog::init, this));
	}

	grid_ptr find_grid(new gui::grid(2));
	find_grid->add_col(widget_ptr(new label("Search: ", graphics::color_white())));
	find_grid->add_col(widget_ptr(find_edit_));
	add_widget(find_grid, 10, 10);

	const frame& frame = *editor_.all_characters()[editor_.get_object()].preview_frame();

	button* facing_button = new button(
	  widget_ptr(new label(editor_.face_right() ? "right" : "left", graphics::color_white())),
	  boost::bind(&editor::toggle_facing, &editor_));
	facing_button->set_tooltip("f  Change Facing");
	if(find_edit_->text().empty() == false) {
		add_widget(widget_ptr(facing_button));
		add_widget(generate_grid(""));
	} else {

		button* category_button = new button(widget_ptr(new label(category_, graphics::color_white())), boost::bind(&character_editor_dialog::show_category_menu, this));
		add_widget(widget_ptr(category_button));

		add_widget(generate_grid(category_));
	
		add_widget(widget_ptr(facing_button), category_button->x() + category_button->width() + 10, category_button->y());
	}
}

gui::widget_ptr character_editor_dialog::generate_grid(const std::string& category)
{
	std::cerr << "generate grid: " << category << "\n";
	using namespace gui;
	widget_ptr& result = grids_[category];
	std::vector<gui::border_widget_ptr>& borders = grid_borders_[category];
	if(!result || category == "") {
		const std::string search_string = find_edit_->text();

		grid_ptr grid(new gui::grid(3));
		grid->set_max_height(height() - 50);
		int index = 0;
		foreach(const editor::enemy_type& c, editor_.all_characters()) {
			bool matches = c.category == category;
			if(search_string.empty() == false) {
				const std::string id = module::get_id(c.node["type"].as_string());
				const char* p = strstr(id.c_str(), search_string.c_str());
				matches = p == id.c_str() || p != NULL && *(p-1) == '_';
			}

			if(matches) {
				if(first_obj_.count(category_) == 0) {
					first_obj_[category] = index;
				}

				image_widget* preview = new image_widget(c.preview_frame()->img());
				preview->set_dim(36, 36);
				preview->set_area(c.preview_frame()->area());
				button_ptr char_button(new button(widget_ptr(preview), boost::bind(&character_editor_dialog::set_character, this, index)));
	
				std::string tooltip_str = c.node["type"].as_string();

				if(c.help.empty() == false) {
					tooltip_str += "\n" + c.help;
				}
				char_button->set_tooltip(tooltip_str);
				char_button->set_dim(40, 40);
				borders.push_back(new gui::border_widget(char_button, graphics::color(0,0,0,0)));
				grid->add_col(gui::widget_ptr(borders.back()));
			} else {
				borders.push_back(NULL);
			}
	
			++index;
		}

		grid->finish_row();

		result = grid;
	}

	for(int n = 0; n != borders.size(); ++n) {
		if(!borders[n]) {
			continue;
		}
		borders[n]->set_color(n == editor_.get_object() ? graphics::color(255,255,255,255) : graphics::color(0,0,0,0));
	}
	std::cerr << "done generate grid: " << category << "\n";

	return result;
}

void character_editor_dialog::show_category_menu()
{
	using namespace gui;
	gui::grid* grid = new gui::grid(2);
	grid->set_max_height(height());
	grid->set_show_background(true);
	grid->set_hpad(10);
	grid->allow_selection();
	grid->register_selection_callback(boost::bind(&character_editor_dialog::close_context_menu, this, _1));

	std::map<std::string, const editor::enemy_type*> categories;
	foreach(const editor::enemy_type& c, editor_.all_characters()) {
		std::string category = c.category;
		foreach(char& c, category) {
			c = tolower(c);
		}

		if(categories.count(category)) {
			continue;
		}

		categories[category] = &c;
	}

	typedef std::pair<std::string, const editor::enemy_type*> cat_pair;
	foreach(const cat_pair& cp, categories) {
		const editor::enemy_type& c = *cp.second;

		image_widget* preview = new image_widget(c.preview_frame()->img());
		preview->set_dim(28, 28);
		preview->set_area(c.preview_frame()->area());
		grid->add_col(widget_ptr(preview))
		     .add_col(widget_ptr(new label(c.category, graphics::color_white())));
		grid->register_row_selection_callback(boost::bind(&character_editor_dialog::select_category, this, c.category));
	}

	int mousex, mousey;
	SDL_GetMouseState(&mousex, &mousey);
	if(mousex + grid->width() > graphics::screen_width()) {
		mousex = graphics::screen_width() - grid->width();
	}

	if(mousey + grid->height() > graphics::screen_height()) {
		mousey = graphics::screen_height() - grid->height();
	}

	mousex -= x();
	mousey -= y();

	remove_widget(context_menu_);
	context_menu_.reset(grid);
	add_widget(context_menu_, mousex - 20, mousey);
}

void character_editor_dialog::set_character(int index)
{
	category_ = editor_.all_characters()[index].category;
	editor_.set_object(index);
	init();
}

void character_editor_dialog::close_context_menu(int index)
{
	remove_widget(context_menu_);
	context_menu_.reset();
}

void character_editor_dialog::select_category(const std::string& category)
{
	std::cerr << "SELECT CATEGORY: " << category << "\n";
	category_ = category;
	init();
	set_character(first_obj_[category_]);
}

}
#endif // NO_EDITOR
