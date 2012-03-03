#include <list>
#include <sstream>

#include "custom_object.hpp"
#include "custom_object_functions.hpp"
#include "draw_scene.hpp"
#include "font.hpp"
#include "formatter.hpp"
#include "debug_console.hpp"
#include "foreach.hpp"
#include "level.hpp"
#include "player_info.hpp"
#include "preferences.hpp"
#include "raster.hpp"
#include "text_entry_widget.hpp"

namespace debug_console
{

namespace {
std::list<graphics::texture>& messages() {
	static std::list<graphics::texture> message_queue;
	return message_queue;
}
}

void add_message(const std::string& msg)
{
	if(!preferences::debug()) {
		return;
	}

	if(msg.size() > 100) {
		std::string trunc_msg(msg.begin(), msg.begin() + 90);
		trunc_msg += "...";
		add_message(trunc_msg);
		return;
	}

	const SDL_Color col = {255, 255, 255, 255};
	try {
		messages().push_back(font::render_text(msg, col, 14));
	} catch(font::error& e) {

		std::cerr << "FAILED TO ADD MESSAGE DUE TO FONT RENDERING FAILURE\n";
		return;
	}
	if(messages().size() > 8) {
		messages().pop_front();
	}
}

void draw()
{
	if(messages().empty()) {
		return;
	}

	int ypos = 80;
	foreach(const graphics::texture& t, messages()) {
		const SDL_Rect area = {0, ypos-2, t.width() + 10, t.height() + 5};
		graphics::draw_rect(area, graphics::color_black(), 128);
		graphics::blit_texture(t, 5, ypos);
		ypos += t.height() + 5;
	}
}

namespace {
#if !TARGET_IPHONE_SIMULATOR && !TARGET_OS_IPHONE
class console
{
public:
	explicit console()
	  : history_pos_(0) {
		entry_.set_font("door_label");
		entry_.set_loc(10, 300);
		entry_.set_dim(300, 20);
	}

	void execute(level& lvl, entity& ob) const {
		history_.clear();
		history_pos_ = 0;

		entity_ptr context(&ob);
		lvl.editor_select_object(context);

		bool done = false;
		while(!done) {
			int mousex, mousey;
			const unsigned int buttons = SDL_GetMouseState(&mousex, &mousey);

			entity_ptr selected = lvl.get_next_character_at_point(last_draw_position().x + mousex, last_draw_position().y + mousey, last_draw_position().x, last_draw_position().y);
			lvl.set_editor_highlight(selected);

			SDL_Event event;
			while(SDL_PollEvent(&event)) {
				switch(event.type) {
				case SDL_MOUSEBUTTONDOWN: {
					if(selected) {
						context = selected;
						lvl.editor_clear_selection();
						lvl.editor_select_object(context);
						debug_console::add_message(formatter() << "Selected object: " << selected->debug_description());
					}
					break;
				}

				case SDL_KEYDOWN:
					done = event.key.keysym.sym == SDLK_ESCAPE;

					if(event.key.keysym.sym == SDLK_RETURN) {
						try {
							const std::string text = entry_.text();
							history_.push_back(entry_.text());
							history_pos_ = history_.size();
							entry_.set_text("");
							game_logic::formula f(text, &get_custom_object_functions_symbol_table());
							variant v = f.execute(*context);
							context->execute_command(v);
							debug_console::add_message(v.to_debug_string());
						} catch(game_logic::formula_error&) {
							debug_console::add_message("error parsing formula");
						} catch(...) {
							debug_console::add_message("unknown error parsing formula");
						}
					} else if(event.key.keysym.sym == SDLK_UP) {
						if(history_pos_ > 0) {
							--history_pos_;
							ASSERT_LT(history_pos_, history_.size());
							entry_.set_text(history_[history_pos_]);
						}
					} else if(event.key.keysym.sym == SDLK_DOWN) {
						if(!history_.empty() && history_pos_ < history_.size() - 1) {
							++history_pos_;
							entry_.set_text(history_[history_pos_]);
						} else {
							history_pos_ = history_.size();
							entry_.set_text("");
						}
					}
					break;
				}

				entry_.process_event(event, false);
			}

			draw(lvl);
			SDL_Delay(20);
		}
	}
private:
	void draw(const level& lvl) const {
		draw_scene(lvl, last_draw_position(), &lvl.player()->get_entity());

		entry_.draw();

		SDL_GL_SwapBuffers();
	}

	mutable gui::text_entry_widget entry_;
	mutable std::vector<std::string> history_;
	mutable int history_pos_;
};

}

void show_interactive_console(level& lvl, entity& obj)
{
	console().execute(lvl, obj);
}

#else
void show_interactive_console(level& lvl, entity& obj) {}
#endif

}
